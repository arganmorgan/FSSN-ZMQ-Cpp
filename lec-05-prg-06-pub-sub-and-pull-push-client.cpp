#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <zmq.hpp>

int main() {
    // Prepare our context and subscriber
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, zmq::socket_type::sub);
    subscriber.set(zmq::sockopt::subscribe, "");
    subscriber.connect("tcp://localhost:5557");
    zmq::socket_t publisher(context, zmq::socket_type::push);
    publisher.connect("tcp://localhost:5558");

    srand(time(NULL));
    while (true) {
        zmq::pollitem_t items[] = {
            { subscriber.handle(), 0, ZMQ_POLLIN, 0 }
        };
        zmq::poll(items, 1, std::chrono::milliseconds(100));
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t message;
            subscriber.recv(message, zmq::recv_flags::none);
            std::string msg_str(static_cast<char*>(message.data()), message.size());
            std::cout << "I: received message  '" << msg_str << "'\n";
        }
        else {
            int r = rand() % 100 + 1;
            if (r < 10) {
                publisher.send(zmq::buffer(std::to_string(r)), zmq::send_flags::none);
                std::cout << "I: sending message  " << r << std::endl;
            }
        }
    }
}