#ifndef HW_COMMS_HPP
#define HW_COMMS_HPP

#include <string>
#include "errors.hpp"
#include <map>

#define DEBUG_SIMPLE_DISCOVERY_MODE // Uncomment this to use simplified discovery for VM work

typedef enum {
    DC_UNKNOWN_DEVICE_CMD = 0,
    DC_HANDSHAKE_HELLO,
    DC_GET_SERIAL_NUMBER,
    DC_GET_FIRMWARE_VERSION,
    DC_GET_PRIVATE_KEY
} deviceCmd;

/**
 * @brief Maps from command names to the string that we actually send to the USB key
 * to implement that command
 */ 
const std::map<deviceCmd, std::string> deviceCmdMap = {
    {DC_HANDSHAKE_HELLO, "pc_hello\n"},
    {DC_GET_SERIAL_NUMBER, "pc_req_sn\n"},
    {DC_GET_FIRMWARE_VERSION, "pc_req_fw\n"},
    {DC_GET_PRIVATE_KEY, "pc_req_key\n"}
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