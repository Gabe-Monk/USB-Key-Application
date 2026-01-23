#include <stdio.h>
#include <zmq.hpp>
#include "messages.hpp"

int main() {
    // Setup ZMQ
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://*:5555");

    printf("Device communication server listening on port 5555...\n");

    // Variable to track if finger is enrolled
    bool is_enrolled = false;

    while (true) {
        zmq::message_t request;
        
        // Wait for a message
        socket.recv(request, zmq::recv_flags::none);

        CmdMsg msg(request);

        // Build response
        Json::Value response;
        response["req_id"] = msg.getReqId();
        response["ok"] = true;
        
        Json::Value data;

        switch (msg.getCmd()) {
            case userCmd::GET_STATUS:
                data["serial"] = "HW-DEVICE-001";
                data["firmware"] = "1.0.0";
                data["fingerprint_enrolled"] = is_enrolled;
                break;
            case userCmd::WD_HEARTBEAT:
                data["device_connected"] = true;
                break;
            case userCmd::ENROLL_FINGERPRINT:
            case userCmd::AUTH_FINGERPRINT:
            case userCmd::GET_SECRET_KEY:
            default:
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
}