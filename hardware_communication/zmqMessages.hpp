#ifndef ZMQMESSAGES_HPP
#define ZMQMESSAGES_HPP

#include <zmq.hpp>
#include <jsoncpp/json/json.h>
#include <stdio.h>
#include "errors.hpp"

// Expected commands received from main program
typedef enum {
    UNKNOWN_USER_CMD = 0,
    GET_STATUS,
    ENROLL_FINGERPRINT,
    AUTH_FINGERPRINT,
    GET_SECRET_KEY,
    WD_HEARTBEAT
} userCmd;

const std::map<std::string, userCmd> userCmdMap = {
    {"GET_STATUS", GET_STATUS},
    {"ENROLL_FINGERPRINT", ENROLL_FINGERPRINT},
    {"AUTH_FINGERPRINT", AUTH_FINGERPRINT},
    {"GET_SECRET_KEY", GET_SECRET_KEY},
    {"WD_HEARTBEAT", WD_HEARTBEAT}
};

/**
 * @brief Translates string from received ZMQ message to appropriate userCmd value
 * 
 * @param cmdStr String detailing the command type
 * 
 * @return Equivalent `userCmd` to `cmdStr`
 */
userCmd stringToUserCmd(const std::string& cmdStr) {
    auto it = userCmdMap.find(cmdStr);
    if (it != userCmdMap.end()) {
        return it->second;
    }
    return UNKNOWN_USER_CMD;
}

/**
 * @brief Translates userCmd value to appropriate string
 */
std::string userCmdToStr (const userCmd cmd) {
    auto it = std::find_if(userCmdMap.begin(), userCmdMap.end(), [cmd](const auto& kv) {return kv.second == cmd;});
    if (it != userCmdMap.end()) {
        return it->first;
    }
    return "UNKNOWN";
}

/**
 * @brief ZMQ messages from main program to hw_comms submodule
 */
class CmdMsg {
    private:
        uint32_t reqId = 0;
        userCmd cmd;
        Json::Value data;

    public:
        CmdMsg() = default;

        CmdMsg(const Json::Value& root) : CmdMsg() {
            deserialize(root);
        }

        CmdMsg(const zmq::message_t& req) : CmdMsg() {
            // Turn message into a string
            std::string msg(static_cast<const char*>(req.data()), req.size());

            // Parse JSON
            Json::Value reqJson;
            Json::Reader reader;

            if (!reader.parse(msg, reqJson)) {
                LOG_ERROR("JSON parse error - %s", reader.getFormattedErrorMessages().c_str());
                return;
            }
            
            deserialize(reqJson);
        }

        void deserialize(const Json::Value& root) {
            if (root.isMember("req_id")) {
                reqId = root["req_id"].asUInt();
            }

            if (root.isMember("cmd")) {
                cmd = stringToUserCmd(root["cmd"].asString());
            }

            if (root.isMember("data")) {
                data = root["data"];
            }
        }

        uint32_t getReqId () {return reqId;}
        userCmd getCmd () {return cmd;}
};

#endif