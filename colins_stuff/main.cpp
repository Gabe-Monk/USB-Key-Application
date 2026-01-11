#include <stdio.h>
#include <libserialport.h>
#include <string>
#include <unistd.h> // For sleep()
#include <vector>
#include <algorithm>
#include <iostream> // For pop_back()
#include <cstring> // For strlen

#define OK 0
#define ERROR_GENERIC           -1
#define ERROR_FAILED_HANDSHAKE  -2

int getNewSerialPort(std::string &portName);
void trimTrailingNewlines(std::string& s);
void trimTrailingNewlines(char *s);
int sendToKey(std::string msg, struct sp_port *port);
int readFromKey(std::string &msg, struct sp_port *port);

int main () {
    // Start by getting name of serial port we have the key plugged into
    std::string serialPortName;
    int ret = getNewSerialPort(serialPortName);
    if (ret != OK) {
        fprintf(stderr, "ERROR (%d) - main: Could not find target serial port. Make sure device is disconnected when program starts\n", ret);
        return ret;
    }
    printf("USB key detected on interface '%s'\n", serialPortName.c_str());
    
    // ret = readFromKey(serialPortName);
    // std::string serialPortName = "/dev/ttyACM0"; // DELETEME

    // Open connection with that serial port
    struct sp_port *port;

    ret = sp_get_port_by_name(serialPortName.c_str(), &port);
    if (ret != SP_OK) {
        fprintf(stderr, "ERROR (%d) - main: Failed to find port with name '%s'\n", ret, serialPortName.c_str());
        sp_free_port(port);
        return ret;
    }

    ret = sp_open(port, SP_MODE_READ_WRITE);
    if (ret != SP_OK) {
        fprintf(stderr, "ERROR (%d) - main: Failed to open serial port '%s'\n", ret, serialPortName.c_str());
        sp_free_port(port);
        return ret;
    }

    // Start handshake
    sendToKey("pc_hello\n", port);
    std::string reply;
    readFromKey(reply, port);
    if (reply != "usb_key_hello") {
        ret = ERROR_FAILED_HANDSHAKE;
        fprintf(stderr, "ERROR (%d) - main: Didn't receive hello message back from serial port. Received '%s'\n", ret, reply.c_str());
        sp_close(port);
        sp_free_port(port);
        return ret;
    }

    sendToKey("pc_req_sn\n", port);
    readFromKey(reply, port);


    
    sp_close(port);
    sp_free_port(port);

    return OK;
}

void trimTrailingNewlines(std::string& s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        s.pop_back();
    }
}

void trimTrailingNewlines(char *s) {
    if (!s) return;  // safety check

    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';  // remove last character
        len--;
    }
}

int getNewSerialPort(std::string &portName) {
    struct sp_port **ports;
    std::vector<std::string> portNamesOg;
    std::vector<std::string> portNamesNew;

    printf("Detecting USB interfaces present prior to insertion of USB key...\n");

    int ret = sp_list_ports(&ports);
    if (ret != SP_OK) {
        fprintf(stderr, "ERROR (%d) - getNewSerialPort: Failed to list serial ports via sp_list_ports\n", ret);
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

        ret = sp_list_ports(&ports);
        if (ret != SP_OK) {
            fprintf(stderr, "ERROR (%d) - getNewSerialPort: Failed to list serial ports via sp_list_ports\n", ret);
            sp_free_port_list(ports);
            return ret;
        }

        // Check sizes to see if we've found a new device
        int newPorts;
        for (newPorts = 0; ports[newPorts]; newPorts++) {}
        if (newPorts == portNamesOg.size()+1) {
            break;
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
        fprintf(stderr, "ERROR (%d) - getNewSerialPort: Expected to find 1 additional interface after device was plugged in, actually found %d (%lu before, %lu after)\n", ret, (int)portNamesNew.size()-(int)portNamesOg.size(), portNamesOg.size(), portNamesNew.size());
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
    fprintf(stderr, "ERROR (%d) - getNewSerialPort\n", ret);
    return ret;
}

int sendToKey(std::string msg, struct sp_port *port) {
    int bytes_written = sp_nonblocking_write(port, msg.c_str(), msg.size());
    trimTrailingNewlines(msg); // Just to erase newline when we print here
    printf("Sent: %s\n", msg.c_str());
    return OK;
}

int readFromKey(std::string &msg, struct sp_port *port) {
    // Receive comms from Pico
    // char buf[100];
    // int bytes_read = sp_blocking_read(port, buf, sizeof(buf), 5000); // 5 sec timeout
    char buf[512];
    int bytes_read = 0;
    bool receivedNewline = false;

    while (bytes_read < sizeof(buf)) {
        char c;
        int n = sp_blocking_read(port, &c, 1, 100);  // read 1 byte at a time
        if (n <= 0) {
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
        printf("Received: %s\n", buf);
    } else {
        printf("No data received\n");
    }

    return OK;
}