#include <zmq.hpp>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

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

    std::cout << "C++ server listening (Stateful Mode)...\n";

    // 1. Create a state variable
    bool is_enrolled = false; 

    while (true) {
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none);

        std::string msg(static_cast<char*>(request.data()), request.size());
        std::cout << "Received: " << msg << std::endl;

        std::string req_id = get_json_value(msg, "req_id");
        std::string cmd = get_json_value(msg, "cmd");

        std::string reply_data = "{}";

        // 2. Handle Commands Dynamically
        if (cmd == "STATUS") {
            // Return the current state of is_enrolled
            std::string enrolled_str = is_enrolled ? "true" : "false";
            reply_data = "{ \"serial\": \"CPP-TEST\", \"firmware\": \"1.0\", \"fingerprint_enrolled\": " + enrolled_str + " }";
        }
        else if (cmd == "ENROLL_FINGERPRINT") {
            std::cout << "Enrolling fingerprint..." << std::endl;
            // Simulate time taken to enroll
            std::this_thread::sleep_for(std::chrono::seconds(2));
            is_enrolled = true; // Update state
            reply_data = "{}"; 
        }
        else if (cmd == "AUTH_FINGERPRINT") {
             if (is_enrolled) {
                 reply_data = "{ \"accepted\": true, \"elapsed_ms\": 500 }";
             } else {
                 reply_data = "{ \"accepted\": false, \"reason\": \"not enrolled\" }";
             }
        }
        else if (cmd == "GET_SECRET_KEY") {
             // Fake base64 key
             reply_data = "{ \"secret_key_b64\": \"MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDE=\" }";
        }

        // 3. Send Reply
        std::string reply = "{";
        reply += "\"req_id\": \"" + req_id + "\", ";
        reply += "\"ok\": true, ";
        reply += "\"data\": " + reply_data;
        reply += "}";

        socket.send(zmq::buffer(reply), zmq::send_flags::none);
    }
}
