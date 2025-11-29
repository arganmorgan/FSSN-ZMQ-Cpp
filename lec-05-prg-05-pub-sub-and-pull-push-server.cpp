#include <iostream>
#include <string>
#include <zmq.hpp>

int main() {
    // context and sockets
    zmq::context_t ctx(1);
    zmq::socket_t publisher(ctx, zmq::socket_type::pub);
    publisher.bind("tcp://*:5557");
    zmq::socket_t collector(ctx, zmq::socket_type::pull);
    collector.bind("tcp://*:5558");

    while (true) {
        zmq::message_t message;
        collector.recv(message, zmq::recv_flags::none);
        std::string msg_str(static_cast<char*>(message.data()), message.size());
        std::cout << "I: publishing update  '" << msg_str << "'\n";
        publisher.send(message, zmq::send_flags::none);
    }
}