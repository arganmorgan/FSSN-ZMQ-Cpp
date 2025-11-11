#include <zmq.hpp>
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://*:5555");

    while (true) {
        //  Wait for next request from client
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none);
        std::string message(static_cast<char*>(request.data()), request.size());
        std::cout << "Received request: " << message << std::endl;

        //  Do some 'work'
        std::this_thread::sleep_for(std::chrono::seconds(1));

        //  Send reply back to client
        socket.send(zmq::buffer("World"), zmq::send_flags::none);
    }
}