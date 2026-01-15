#include <zmq.hpp>
#include <string>
#include <iostream>

// Helper to find the value of a JSON field manually (hacky, but works for testing)
std::string get_json_value(std::string json, std::string key) {
    std::string key_pattern = "\"" + key + "\":";
    size_t start = json.find(key_pattern);
    if (start == std::string::npos) return "";
    
    start += key_pattern.length();
    
    // Skip whitespace/quotes
    while (start < json.length() && (json[start] == ' ' || json[start] == '\"')) {
        start++;
    }
    
    size_t end = start;
    while (end < json.length() && json[end] != '\"' && json[end] != ',' && json[end] != '}') {
        end++;
    }
    
    return json.substr(start, end - start);
}

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://*:5555");

    std::cout << "C++ server listening (JSON Protocol Mode)...\n";

    while (true) {
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none);

        std::string msg(static_cast<char*>(request.data()), request.size());
        std::cout << "Received: " << msg << std::endl;

        // 1. Extract the Request ID so we can send it back
        std::string req_id = get_json_value(msg, "req_id");
        std::string cmd = get_json_value(msg, "cmd");

        // 2. Construct a valid JSON reply
        // We just say "ok": true for everything for this test
        std::string reply = "{";
        reply += "\"req_id\": \"" + req_id + "\", ";
        reply += "\"ok\": true, ";
        reply += "\"data\": { \"serial\": \"CPP-TEST\", \"firmware\": \"1.0\", \"fingerprint_enrolled\": true }";
        reply += "}";

        socket.send(zmq::buffer(reply), zmq::send_flags::none);
    }
}
