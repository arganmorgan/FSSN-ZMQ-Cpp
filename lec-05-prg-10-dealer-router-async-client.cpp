#include <zmq.hpp>
#include <thread>
#include <iostream>
#include <string>
#include <chrono>

class ClientTask {
public:
    ClientTask(const std::string& id)
        : id(id) {
    }

    void run() {
        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::dealer);
        std::string identity = id;
        socket.set(zmq::sockopt::routing_id, identity);
        socket.connect("tcp://localhost:5570");
        std::cout << "Client " << identity << " started" << std::endl;
        zmq::pollitem_t items[] = {
            { socket, 0, ZMQ_POLLIN, 0 }
        };
        int reqs = 0;
        while (true) {
            reqs = reqs + 1;
            std::cout << "Req #" << reqs << " sent.." << std::endl;
            std::string msg = "request #" + std::to_string(reqs);
            socket.send(zmq::buffer(msg), zmq::send_flags::none);

            std::this_thread::sleep_for(std::chrono::seconds(1));
            zmq::poll(items, 1, std::chrono::milliseconds(1000));
            if (items[0].revents & ZMQ_POLLIN) {
                zmq::message_t recv_msg;
                socket.recv(recv_msg, zmq::recv_flags::none);
                std::string msg_str(static_cast<char*>(recv_msg.data()), recv_msg.size());
                std::cout << identity << " received: " << msg_str << std::endl;
            }
        }

        socket.close();   // useless
        context.close();  // useless
    }

private:
    std::string id;
};

int main(int argc, char* argv[]) {
    ClientTask client(argv[1]);
    client.run();
}