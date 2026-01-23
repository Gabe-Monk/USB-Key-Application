#include <zmq.hpp>
#include <jsoncpp/json/json.h>
#include <stdio.h>

// Expected commands received from main program
typedef enum {
    UNKNOWN = 0,
    GET_STATUS,
    ENROLL_FINGERPRINT,
    AUTH_FINGERPRINT,
    GET_SECRET_KEY,
    WD_HEARTBEAT
}userCmd;

/**
 * @brief Translates string from received ZMQ message to appropriate userCmd value
 * 
 * @param cmdStr String detailing the command type
 * 
 * @return Equivalent `userCmd` to `cmdStr`
 */
userCmd stringToUserCmd(const std::string& cmdStr) {
    static const std::map<std::string, userCmd> cmdMap = {
        {"GET_STATUS", GET_STATUS},
        {"ENROLL_FINGERPRINT", ENROLL_FINGERPRINT},
        {"AUTH_FINGERPRINT", AUTH_FINGERPRINT},
        {"GET_SECRET_KEY", GET_SECRET_KEY},
        {"WD_HEARTBEAT", WD_HEARTBEAT}
    };
    
    auto it = cmdMap.find(cmdStr);
    if (it != cmdMap.end()) {
        return it->second;
    }
    return UNKNOWN;
}

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
                fprintf(stderr, "Error: CmdMsg - JSON parse error - %s\n", reader.getFormattedErrorMessages().c_str());
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