#include <zmq.hpp>
#include <thread>
#include <iostream>
#include <string>
#include <chrono>

class ClientTask {
public:
    ClientTask(const std::string& id)
        : id(id), context(1), socket(context, zmq::socket_type::dealer) { }

    void recvHandler() {
        while (true) {
            zmq::pollitem_t items[] = {
                { static_cast<void*>(socket), 0, ZMQ_POLLIN, 0 }
            };
            zmq::poll(items, 1, std::chrono::milliseconds(1000));
            if (items[0].revents & ZMQ_POLLIN) {
                zmq::message_t msg;
                socket.recv(msg);
                std::string msg_str(static_cast<char*>(msg.data()), msg.size());
                std::cout << identity << " received: " << msg_str << std::endl;
            }
        }
    }

    void run() {
        identity = id;
        socket.set(zmq::sockopt::routing_id, identity);
        socket.connect("tcp://localhost:5570");
        std::cout << "Client " << identity << " started\n";
        int reqs = 0;

        std::thread clientThread(&ClientTask::recvHandler, this);
        clientThread.detach();

        while (true) {
            reqs = reqs + 1;
            std::cout << "Req #" << reqs << " sent..\n";
            socket.send(zmq::buffer("request #" + std::to_string(reqs)));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        socket.close();   // useless
        context.close();  // useless
    }

private:
    std::string id;
    std::string identity;
    zmq::context_t context;
    zmq::socket_t socket;
};

int main(int argc, char* argv[]) {
    ClientTask client(argv[1]);
    client.run();
}