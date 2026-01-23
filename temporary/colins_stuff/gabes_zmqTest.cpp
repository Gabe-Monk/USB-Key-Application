#include <zmq.hpp>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

// Function to find value in JSON string.
// Done manually to avoid installing a JSON library.
std::string get_json_value(std::string json, std::string key) {
    std::string key_pattern = "\"" + key + "\":";
    size_t start = json.find(key_pattern);
    if (start == std::string::npos) return "";
    
    start += key_pattern.length();
    
    // Skip spaces and quotes
    while (start < json.length() && (json[start] == ' ' || json[start] == '\"')) {
        start++;
    }
    
    size_t end = start;
    // Read until the end of the value
    while (end < json.length() && json[end] != '\"' && json[end] != ',' && json[end] != '}') {
        end++;
    }
    
    return json.substr(start, end - start);
}

int main() {
    // Setup ZMQ
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://*:5555");

    std::cout << "Server starting on port 5555...\n";

    // Variable to track if finger is enrolled
    bool is_enrolled = false; 

    while (true) {
        zmq::message_t request;
        
        // Wait for a message
        socket.recv(request, zmq::recv_flags::none);

        // Turn message into a string
        std::string msg(static_cast<char*>(request.data()), request.size());
        std::cout << "Got message: " << msg << std::endl;

        // Get info from the message
        std::string req_id = get_json_value(msg, "req_id");
        std::string cmd = get_json_value(msg, "cmd");

        std::string reply_data = "{}";

        // Check which command it is
        if (cmd == "STATUS") {
            // Check if enrolled is true or false
            std::string enrolled_str = is_enrolled ? "true" : "false";
            reply_data = "{ \"serial\": \"CPP-TEST\", \"firmware\": \"1.0\", \"fingerprint_enrolled\": " + enrolled_str + " }";
        }
        else if (cmd == "ENROLL_FINGERPRINT") {
            std::cout << "Enrolling..." << std::endl;
            
            // Wait 2 seconds to fake the scan time
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            is_enrolled = true; 
            reply_data = "{}"; 
        }
        else if (cmd == "AUTH_FINGERPRINT") {
             // Only work if already enrolled
             if (is_enrolled) {
                 reply_data = "{ \"accepted\": true, \"elapsed_ms\": 500 }";
             } else {
                 reply_data = "{ \"accepted\": false, \"reason\": \"not enrolled\" }";
             }
        }
        else if (cmd == "GET_SECRET_KEY") {
             // Send a fake key
             reply_data = "{ \"secret_key_b64\": \"MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDE=\" }";
        }

        // Build the reply string
        std::string reply = "{";
        reply += "\"req_id\": \"" + req_id + "\", ";
        reply += "\"ok\": true, ";
        reply += "\"data\": " + reply_data;
        reply += "}";

        // Send it back
        socket.send(zmq::buffer(reply), zmq::send_flags::none);
    }
}