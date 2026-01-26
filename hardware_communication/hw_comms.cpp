#include <stdio.h>
#include "hw_comms.hpp"
#include "zmqMessages.hpp" // For comms with main program
#include <zmq.hpp> // For comms with main program
#include <libserialport.h> // For comms with device
#include <unistd.h> // For sleep()
#include <vector>
#include <algorithm>
#include <iostream> // For pop_back()
#include <cstring> // For strlen

int main() {
    // Setup ZMQ
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://*:5555");

    LOG_DBG("Device communication server listening on port 5555...");

    // Setup comms with device
    struct sp_port *port;
    deviceErr ret = initDeviceComms(&port);
    if (ret != OK) {
        LOG_ERR(ret, "Failed to initiate communications with device");
        return ret;
    }

    // Start handshake
    CHK(performHandshake(port));

    LOG_DBG("Handshake completed");

    // Main loop that listens for messages from Python program and replies to them
    while (true) {
        zmq::message_t request;
        
        // Wait for a message
        socket.recv(request, zmq::recv_flags::none);
        CmdMsg msg(request);

        // Don't bother logging heartbeats
        if (msg.getCmd() != WD_HEARTBEAT) {
            LOG_DBG("Got message (type %s)", userCmdToStr(msg.getCmd()).c_str());
        }

        // Build response
        Json::Value response;
        response["req_id"] = msg.getReqId(); // Response will carry same ID as request so that they can be associated if we want to implement something like that
        
        Json::Value data;

        switch (msg.getCmd()) {
            case GET_STATUS: {
                std::string reply;
                CHK(getSerialNumber(reply, port));
                data["serial"] = reply;
                CHK(getFirmware(reply, port));
                data["firmware"] = reply;
                data["fingerprint_enrolled"] = false; // TODO:
                break;
            }
            case ENROLL_FINGERPRINT: // TODO:
                break; 
            case AUTH_FINGERPRINT: // TODO:
                break;
            case GET_SECRET_KEY: // TODO:
                break;
            case WD_HEARTBEAT:
                data["device_connected"] = true; // TODO:
                break;
            default: // TODO: We should implement some handling for this scenario on both ends of the communication. I've put some boilerplate here for now
                response["ok"] = false;
                data["error"] = "Unknown command";
                break;
        }

        response["data"] = data;

        // Convert to string and send
        Json::StreamWriterBuilder builder;
        std::string reply = Json::writeString(builder, response);
        socket.send(zmq::buffer(reply), zmq::send_flags::none);
    }

    sp_close(port);
    sp_free_port(port);

    return OK;
}

/*********************************************************************************************************************/
void trimTrailingNewlines(std::string& s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        s.pop_back();
    }
}

/*********************************************************************************************************************/
void trimTrailingNewlines(char *s) {
    if (!s) return;  // safety check

    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';  // remove last character
        len--;
    }
}

/*********************************************************************************************************************
 * @brief Finds the port we want to talk to the USB key on by instructing the user to plug in the key, which creates 
 * a not-previously-discoverable USB device.
 * 
 * @param portName This string will be set to the name of the port that we will talk to the USB key over 
 * (e.g., '/dev/ttyACM0')
 * 
 * @return `OK` if successful
 */
deviceErr getNewSerialPort(std::string &portName) {
    struct sp_port **ports;
    std::vector<std::string> portNamesOg;
    std::vector<std::string> portNamesNew;
    deviceErr ret = OK;

    LOG_DBG("Detecting USB interfaces present prior to insertion of USB key...");

    int spRet = sp_list_ports(&ports);
    if (spRet != SP_OK) {
        ret = ERROR_SERIAL_PORT;
        LOG_ERR(ret, "Failed to list serial ports via sp_list_ports");
        sp_free_port_list(ports);
        return ret;
    }

    // Collect serial port names
    for (int i = 0; ports[i]; i++) {
        portNamesOg.push_back(sp_get_port_name(ports[i]));
    }

    printf("Plug USB key in now (promptly).\n");

    // Check for new device every second, for 30 seconds
    for (int i=0; i<30; i++) {
        sleep(1);

        spRet = sp_list_ports(&ports);

        if (spRet != SP_OK) {
            ret = ERROR_SERIAL_PORT;
            LOG_ERR(ret, "Failed to list serial ports via sp_list_ports");
            sp_free_port_list(ports);
            return ret;
        }

        // Check sizes to see if we've found a new device
        int newPorts;
        for (newPorts = 0; ports[newPorts]; newPorts++) {}
        if (newPorts == portNamesOg.size()+1) {
            break;
        } else {
            sp_free_port_list(ports);
        }
    }

    // Collect new serial port names
    for (int i = 0; ports[i]; i++) {
        portNamesNew.push_back(sp_get_port_name(ports[i]));
    }

    sp_free_port_list(ports);

    // Make sure list sizes are as expected before bothering to continue
    if (portNamesNew.size() != portNamesOg.size()+1) {
        ret = ERROR_GENERIC;
        LOG_ERR(ret, "Expected to find 1 additional interface after device was plugged in, actually found %d (%lu before, %lu after)", (int)portNamesNew.size()-(int)portNamesOg.size(), portNamesOg.size(), portNamesNew.size());
        return ret;
    }

    // If we get here, we should have a valid serial port to give back
    for (std::string newPortName : portNamesNew) {
        if (std::find(portNamesOg.begin(), portNamesOg.end(), newPortName) == portNamesOg.end()) {
            portName = newPortName;
            return OK;
        }
    }

    ret = ERROR_GENERIC;
    LOG_ERR(ret, "Failed to find valid serial port and connect to USB key device");
    return ret;
}

/*********************************************************************************************************************
 * @brief Detects which communication port should be used to communicate with
 * device and opens that connection.
 * 
 * @param port Double-pointer to the `sp_port` struct that we are using to communicate with the USB key
 * 
 * @returns `OK` if successful
 */
deviceErr initDeviceComms (struct sp_port **port) {
    // Start by getting name of serial port we have the key plugged into
    std::string serialPortName;
    
    deviceErr ret = getNewSerialPort(serialPortName);
    if (ret != OK) {
        LOG_ERR(ret, "Could not find target serial port. Make sure device is disconnected when program starts");
        return ret;
    }
    LOG_DBG("USB key detected on interface '%s'", serialPortName.c_str());

    int spRet = sp_get_port_by_name(serialPortName.c_str(), port);
    if (spRet != SP_OK) {
        ret = ERROR_SERIAL_PORT;
        LOG_ERR(ret, "Failed to find port with name '%s'", serialPortName.c_str());
        sp_free_port(*port);
        return ret;
    }

    spRet = sp_open(*port, SP_MODE_READ_WRITE);
    if (spRet != SP_OK) {
        ret = ERROR_SERIAL_PORT;
        LOG_ERR(ret, "Failed to open serial port '%s'", serialPortName.c_str());
        sp_free_port(*port);
        return ret;
    }

    return ret;
}

/*********************************************************************************************************************
 * @brief Sends command to USB key (3 second timeout)
 * 
 * @param cmd Command that we wish to send to USB key
 * @param port Port that we are using to communicate with USB key
 * 
 * @returns `OK` if successful
 */
deviceErr sendToKey(deviceCmd cmd, struct sp_port *port) {   
    std::string msg = deviceCmdToStr(cmd);
    std::string msgNoNewline = msg;
    trimTrailingNewlines(msgNoNewline); // Just to erase newline when we print here
    deviceErr ret = OK;
    LOG_DBG("Sending: %s", msgNoNewline.c_str());

    // int bytes_written = sp_nonblocking_write(port, msg.c_str(), msg.size());
    int bytes_written = sp_blocking_write(port, msg.c_str(), msg.size(), 3000); // 3 second timeout

    if (bytes_written < 0) {
        ret = ERROR_SERIAL_PORT;
        LOG_ERR(ret, "Failed to send msg: '%s'", msgNoNewline.c_str());
    } else {
        LOG_DBG("Sent %d bytes: %s", bytes_written, msgNoNewline.c_str());
    }
    
    return ret;
}

/*********************************************************************************************************************
 * @brief Reads from USB key and collects returned value. Should only be called when expecting a message.
 * 
 * @param msg Received data written here as a string
 * @param port Port that we're using to communicate with the USB key
 * 
 * @returns `OK` if successful
 */
deviceErr readFromKey(std::string &msg, struct sp_port *port) {
    LOG_DBG("Reading...");
    // Receive comms from Pico
    // char buf[100];
    // int bytes_read = sp_blocking_read(port, buf, sizeof(buf), 5000); // 5 sec timeout
    char buf[512];
    int bytes_read = 0;
    bool receivedNewline = false;
    deviceErr ret = OK;

    while (bytes_read < sizeof(buf)) {
        char c;
        int spRet = sp_blocking_read(port, &c, 1, 100);  // read 1 byte at a time
        if (spRet <= 0) {
            // timeout or error
            break;
        }

        buf[bytes_read++] = c;

        // stop reading once we get newline (except wait for 2 since it sends 2)
        if (c == '\n' || c == '\r') {
            if (receivedNewline) {
                break;
            } else {
                receivedNewline = true;
            }
        }
    }

    if (bytes_read > 0) {
        buf[bytes_read] = '\0';
        trimTrailingNewlines(buf);
        msg = buf;
        LOG_DBG("Received: %s", buf);
    } else {
        ret = ERROR_GENERIC;
        LOG_ERR(ret, "No data received");
    }

    return ret;
}

/*********************************************************************************************************************
 * @brief Performs a VERY simple handshake protocol with the USB key to confirm that it is, in fact, a USB key
 *
 * @param port Port that we're using to communicate with the USB key
 * 
 * @returns `OK` if successful
 */
deviceErr performHandshake(struct sp_port *port) {
    deviceErr ret = OK;

    CHK(sendToKey(HANDSHAKE_HELLO, port));
    std::string reply;
    CHK(readFromKey(reply, port));
    if (reply != "usb_key_hello") {
        ret = ERROR_FAILED_HANDSHAKE;
        LOG_ERR(ret, "Didn't receive hello message back from serial port. Received '%s'", reply.c_str());
        sp_close(port);
        sp_free_port(port);
    }

    return ret;
}

/*********************************************************************************************************************
 * @brief Collects serial number by querying USB key
 * 
 * @param sn Serial number gets read into this variable
 * @param port Port that we're using to communicate with the USB key
 * 
 * @returns `OK` if successful
 */
deviceErr getSerialNumber(std::string &sn, struct sp_port *port) {
    CHK(sendToKey(GET_SERIAL_NUMBER, port));
    CHK(readFromKey(sn, port));
    return OK;
}

/*********************************************************************************************************************
 * @brief Collects firmware version by querying USB key
 * 
 * @param fw Firmware version gets read into this variable
 * @param port Port that we're using to communicate with the USB key
 * 
 * @returns `OK` if successful
 */
deviceErr getFirmware(std::string &fw, struct sp_port *port) {
    CHK(sendToKey(GET_FIRMWARE_VERSION, port));
    CHK(readFromKey(fw, port));
    return OK;
}