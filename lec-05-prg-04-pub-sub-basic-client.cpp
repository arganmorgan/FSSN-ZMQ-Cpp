#include <zmq.hpp>
#include <iostream>
#include <string>
#include <sstream>

int main(int argc, char* argv[]) {
    // Socket to talk to server
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::sub);

    std::cout << "Collecting updates from weather server..." << std::endl;
    socket.connect("tcp://localhost:5556");

    // Subscribe to zipcode, default is NYC, 10001
    std::string zip_filter = (argc > 1) ? argv[1] : "10001";
    socket.set(zmq::sockopt::subscribe, zip_filter);

    // Process 5 updates
    int total_temp = 0;
    int update_nbr;
    for (update_nbr = 0; update_nbr < 20; update_nbr++) {
        zmq::message_t message;
        socket.recv(message, zmq::recv_flags::none);
        std::string string(static_cast<char*>(message.data()), message.size());
        std::string zipcode, temperature, relhumidity;
        std::stringstream ss(string);
        ss >> zipcode >> temperature >> relhumidity;
        total_temp += std::stoi(temperature);
        std::cout << "Receive temperature for zipcode '"
            << zip_filter << "' was " << temperature << " F\n";
    }

    std::cout << "Average temperature for zipcode '"
        << zip_filter << "' was " << (total_temp / update_nbr) << " F\n";
}