#include <zmq.hpp>
#include <iostream>

int main() {
    zmq::context_t context(1);

    //  Socket to talk to server
    std::cout << "Connecting to hello world server¡¦" << std::endl;
    zmq::socket_t socket(context, zmq::socket_type::req);
    socket.connect("tcp://localhost:5555");

    //  Do 10 requests, waiting each time for a response
    for (int request = 0; request < 10; ++request) {
        std::cout << "Sending request " << request << " ¡¦" << std::endl;
        socket.send(zmq::buffer("Hello"), zmq::send_flags::none);

        //  Get the reply
        zmq::message_t reply;
        socket.recv(reply, zmq::recv_flags::none);
        std::string message(static_cast<char*>(reply.data()), reply.size());
        std::cout << "Received reply " << request << " [ " << message << " ]" << std::endl;
    }
}