#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <zmq.hpp>

int main(int argc, char* argv[]) {
    // Prepare our context and subscriber
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, zmq::socket_type::sub);
    subscriber.set(zmq::sockopt::subscribe, "");
    subscriber.connect("tcp://localhost:5557");
    zmq::socket_t publisher(context, zmq::socket_type::push);
    publisher.connect("tcp://localhost:5558");

    std::string clientID = argv[1];
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
            std::cout << clientID << ": receive status => " << msg_str << std::endl;
        }
        else {
            int r = rand() % 100 + 1;
            if (r < 10) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::string msg = "(" + clientID + ":ON)";
                publisher.send(zmq::buffer(msg));
                std::cout << clientID << ": send status - activated\n";
            }
            else if (r > 90) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::string msg = "(" + clientID + ":OFF)";
                publisher.send(zmq::buffer(msg));
                std::cout << clientID << ": send status - deactivated\n";
            }
        }
    }
}