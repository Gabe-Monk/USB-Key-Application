#include <zmq.hpp>
#include <string>
#include <iostream>

int main() {
    // 1. Create context
    zmq::context_t context(1);

    // 2. Create REP socket
    zmq::socket_t socket(context, zmq::socket_type::rep);

    // 3. Bind to TCP port
    socket.bind("tcp://*:5555");

    std::cout << "C++ server listening on port 5555...\n";

    while (true) {
        // 4. Receive request
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none);

        std::string msg(static_cast<char*>(request.data()), request.size());
        std::cout << "Received: " << msg << std::endl;

        // 5. Send reply
        std::string reply = "Hello from C++";
        socket.send(zmq::buffer(reply), zmq::send_flags::none);
    }
}
