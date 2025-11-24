#include <zmq.hpp>
#include <string>
#include <iostream>
#include <cstdlib>
#include <ctime>

int main() {
    srand(time(NULL));

    std::cout << "Publishing updates at weather server..." << std::endl;

    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::pub);
    socket.bind("tcp://*:5556");

    while (true) {
        int zipcode = rand() % 99999 + 1;
        int temperature = rand() % 215 - 80;
        int relhumidity = rand() % 50 + 10;

        socket.send(
            zmq::buffer(
                std::to_string(zipcode) + " " +
                std::to_string(temperature) + " " +
                std::to_string(relhumidity)),
            zmq::send_flags::none);
    }
}