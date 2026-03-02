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
        if (msg.getCmd() != UC_WD_HEARTBEAT) {
            LOG_DBG("Got message (type %s)", userCmdToStr(msg.getCmd()).c_str());
        }

        // Build response
        Json::Value response;
        response["req_id"] = msg.getReqId(); // Response will carry same ID as request so that they can be associated if we want to implement something like that
        
        Json::Value data;

        switch (msg.getCmd()) {
            case UC_GET_STATUS: {
                std::string reply;
                CHK(getSerialNumber(reply, port));
                data["serial"] = reply;
                CHK(getFirmware(reply, port));
                data["firmware"] = reply;
                data["fingerprint_enrolled"] = false; // TODO:
                break;
            }
            case UC_ENROLL_FINGERPRINT: { // TODO: NEEDS TESTING
                std::string resp;
                // passing resp to capture response for pc_enroll_fingerprint command
                // NOTE: command is currently very verbose, need to reduce output on Key end
                deviceErr err = addFingerprint(resp, port);
                if (err == OK) {
                    response["ok"] = true;
                }
                else {
                    response["ok"] = false;
                    data["error"] = deviceErrToStr(err);
                }
                break; 
            }
            case UC_AUTH_FINGERPRINT: { // TODO: NEEDS TESTING
                std::string resp;
                // passing resp to capture response for pc_authenticate_fingerprint command
                // NOTE: command is currently very verbose, need to reduce output on Key end
                deviceErr err = authFingerprint(resp, port);
                if (err == OK) {
                    response["ok"] = true;
                }
                else {
                    response["ok"] = false;
                    data["error"] = deviceErrToStr(err);
                }
                break;
            }
            case UC_GET_PRIVATE_KEY: {
                // TODO: Make sure fingerprint confirmed before sending this
                
                std::string reply;
                
                // Fetch the private key from the device by sending the pc_req_key command.
                // The result is stored in the 'reply' string.
                deviceErr err = getPrivateKey(reply, port);
                
                // If the hardware successfully retrieves and transmits the base64 key string:
                if (err == OK) {
                    // Inject the base64 key directly into the JSON data payload 
                    // so the Python script (files.py) can parse it out of the dictionary.
                    data["secret_key"] = reply;
                    response["ok"] = true;
                } else {
                    // Gracefully handle hardware read errors so the main Python process doesn't crash.
                    data["error"] = "Failed to retrieve key";
                    response["ok"] = false;
                }
                break;
            }
            case UC_WD_HEARTBEAT:
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

#ifndef DEBUG_SIMPLE_DISCOVERY_MODE
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
    LOG_DBG("USB key detected on interface '%s'. Attempting to connect...", serialPortName.c_str());

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
#else
/*********************************************************************************************************************
 * @brief Detects which communication port should be used to communicate with
 * device and opens that connection.
 * 
 * @param port Double-pointer to the `sp_port` struct that we are using to communicate with the USB key
 * 
 * @returns `OK` if successful
 */
deviceErr initDeviceComms (struct sp_port **port) {
    // --- CHANGE: HARDCODED PORT FOR VM STABILITY ---
    // Instead of forcing the user to unplug and re-plug the device to detect a new serial interface,
    // we directly target the Linux serial ports standard to the Pico's CDC data console.
    // This is significantly more stable in Docker/WSL/VM environments where USB passthrough can be finicky.
    std::string serialPortName = "/dev/ttyACM0"; 
    
    LOG_DBG("Attempting to connect directly to '%s'...", serialPortName.c_str());

    // Attempt to open the primary port (/dev/ttyACM0)
    int spRet = sp_get_port_by_name(serialPortName.c_str(), port);
    if (spRet != SP_OK) {
        // Fallback: If ACM0 is busy or the OS assigned the Pico to ACM1 (which happens frequently 
        // if the device resets or drops connection briefly), we gracefully fall back to ttyACM1.
        LOG_DBG("Failed to find '%s', trying '/dev/ttyACM1'...", serialPortName.c_str());
        serialPortName = "/dev/ttyACM1";
        spRet = sp_get_port_by_name(serialPortName.c_str(), port);
        
        if (spRet != SP_OK) {
             // If both standard ports fail, the device is completely inaccessible to the container.
             deviceErr ret = ERROR_SERIAL_PORT;
             LOG_ERR(ret, "Failed to find /dev/ttyACM0 or /dev/ttyACM1. Is the device plugged in and passed to the VM?");
             return ret;
        }
    }

    LOG_DBG("USB key found on interface '%s'", serialPortName.c_str());

    // Actually open the connection now that we have successfully located the port structure
    spRet = sp_open(*port, SP_MODE_READ_WRITE);
    if (spRet != SP_OK) {
        deviceErr ret = ERROR_SERIAL_PORT;
        LOG_ERR(ret, "Failed to open serial port '%s'", serialPortName.c_str());
        sp_free_port(*port);
        return ret;
    }

    return OK;
}
#endif

/*********************************************************************************************************************
 * @brief Sends command to USB key (3 second timeout)
 * 
 * @param cmd Command that we wish to send to USB key
 * @param port Port that we are using to communicate with USB key
 * 
 * @returns `OK` if successful
 */
deviceErr sendToKey(deviceCmd cmd, struct sp_port *port) {  
    // Flush any old data from the buffer
    sp_flush(port, SP_BUF_BOTH);
     
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
    // char buf[512];
    // int bytes_read = sp_blocking_read(port, buf, sizeof(buf), 5000); // 5 sec timeout
    char buf[512];
    int bytes_read = 0;
    bool receivedNewline = false;
    deviceErr ret = OK;

    // Use a slightly longer timeout loop for the key retrieval if needed
    // But since sp_blocking_read waits, we rely on its internal timeout.
    while (bytes_read < sizeof(buf) - 1) {
        char c;
        int spRet = sp_blocking_read(port, &c, 1, 3000);  // read 1 byte at a time
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

        std::string unrecognizedErrorPrefix = "error_unrecognized";
        std::string initErrorPrefix = "error_init";
        std::string fingerprintErrorPrefix = "error_fingerprint_sensor";

        // Check for known error codes in read message
        if (msg == "error_invalid_length") {
            ret = ERROR_INVALID_LENGTH;
            LOG_ERR(ret, "Private key had invalid length");
        } else if (msg.compare(0, unrecognizedErrorPrefix.size(), unrecognizedErrorPrefix) == 0) {
            ret = ERROR_UNRECOGNIZED;
            LOG_ERR(ret, "Microcontroller claims: %s", msg.c_str());
        } else if (msg.compare(0, initErrorPrefix.size(), initErrorPrefix) == 0) {
            ret = ERROR_INIT;
            LOG_ERR(ret, "Microcontroller failed initiation: %s", msg.c_str());
        } else if (msg.compare(0, fingerprintErrorPrefix.size(), fingerprintErrorPrefix) == 0) {
            ret = ERROR_FINGERPRINT;
            LOG_ERR(ret, "Fingerprint sensor error: %s", msg.c_str());
        }
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

    CHK(sendToKey(DC_HANDSHAKE_HELLO, port));
    std::string reply;
    CHK(readFromKey(reply, port));
    
    // CHANGED: Use .find() instead of != to ignore trailing newlines/whitespace
    if (reply.find("usb_key_hello") == std::string::npos) {
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
    CHK(sendToKey(DC_GET_SERIAL_NUMBER, port));
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
    CHK(sendToKey(DC_GET_FIRMWARE_VERSION, port));
    CHK(readFromKey(fw, port));
    return OK;
}

/*********************************************************************************************************************/
deviceErr getPrivateKey(std::string &key, struct sp_port *port) {
    CHK(sendToKey(DC_GET_PRIVATE_KEY, port));
    // Increase read timeout logic implicitly by allowing readFromKey to handle it
    CHK(readFromKey(key, port));
    return OK;
}

deviceErr addFingerprint(std::string &resp, struct sp_port *port) {
    CHK(sendToKey(DC_ENROLL_FINGERPRINT, port));
    CHK(readFromKey(resp, port));
    if (resp != "added") {
        return ERROR_FINGERPRINT;
    }
    return OK;
}

deviceErr authFingerprint(std::string &resp, struct sp_port *port) {
    CHK(sendToKey(DC_AUTH_FINGERPRINT, port));
    CHK(readFromKey(resp, port));
    if (resp != "matched") {
        return ERROR_FINGERPRINT;
    }
    return OK;
}
