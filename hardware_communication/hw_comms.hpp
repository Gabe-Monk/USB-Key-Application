#ifndef HW_COMMS_HPP
#define HW_COMMS_HPP

#include <string>
#include "errors.hpp"
#include <map>

typedef enum {
    UNKNOWN_DEVICE_CMD = 0,
    HANDSHAKE_HELLO,
    GET_SERIAL_NUMBER,
    GET_FIRMWARE_VERSION,
    GET_PRIVATE_KEY
} deviceCmd;

/**
 * @brief Maps from command names to the string that we actually send to the USB key
 * to implement that command
 */ 
const std::map<deviceCmd, std::string> deviceCmdMap = {
    {HANDSHAKE_HELLO, "pc_hello\n"},
    {GET_SERIAL_NUMBER, "pc_req_sn\n"},
    {GET_FIRMWARE_VERSION, "pc_req_fw\n"},
    {GET_PRIVATE_KEY, "pc_req_key\n"}
};

/**
 * @brief Translates deviceCmd value to appropriate string to be sent to device
 */
std::string deviceCmdToStr (const deviceCmd cmd) {
    auto it = deviceCmdMap.find(cmd);
    if (it != deviceCmdMap.end()) {
        return it->second;
    }
    return "unknown_command\n";
}

deviceErr initDeviceComms(struct sp_port **port);
deviceErr getNewSerialPort(std::string &portName);
void trimTrailingNewlines(std::string& s);
void trimTrailingNewlines(char *s);
deviceErr sendToKey(deviceCmd cmd, struct sp_port *port);
deviceErr readFromKey(std::string &msg, struct sp_port *port);

deviceErr performHandshake(struct sp_port *port);
deviceErr getSerialNumber(std::string &sn, struct sp_port *port);
deviceErr getFirmware(std::string &fw, struct sp_port *port);
deviceErr getPrivateKey(std::string &key, struct sp_port *port);

#endif