#include <zmq.hpp>
#include <thread>
#include <vector>
#include <iostream>
#include <string>

class ServerWorker {
public:
    ServerWorker(zmq::context_t& context, int id)
        : context(context), id(id) {
    }

    void operator()() {
        zmq::socket_t worker(context, zmq::socket_type::dealer);
        worker.connect("inproc://backend");
        std::cout << "Worker#" << id << " started" << std::endl;
        while (true) {
            zmq::message_t ident;
            zmq::message_t msg;
            worker.recv(ident, zmq::recv_flags::none);
            worker.recv(msg, zmq::recv_flags::none);
            std::string ident_str(static_cast<char*>(ident.data()), ident.size());
            std::string msg_str(static_cast<char*>(msg.data()), msg.size());
            std::cout << "Worker#" << id << " received " << msg_str << " from " << ident_str << std::endl;
            worker.send(ident, zmq::send_flags::sndmore);
            worker.send(msg, zmq::send_flags::none);
        }

        worker.close(); // useless
    }

private:
    zmq::context_t& context;
    int id;
};

class ServerTask {
public:
    ServerTask(int num_server)
        : num_server(num_server) {
    }

    void run() {
        zmq::context_t context(1);
        zmq::socket_t frontend(context, zmq::socket_type::router);
        frontend.bind("tcp://*:5570");

        zmq::socket_t backend(context, zmq::socket_type::dealer);
        backend.bind("inproc://backend");

        std::vector<std::thread> workers;
        for (int i = 0; i < num_server; i++) {
            workers.emplace_back(ServerWorker(context, i));
        }

        zmq::proxy(frontend, backend);

        frontend.close();
        backend.close();
        context.close();
    }

private:
    int num_server;
};

int main(int argc, char* argv[]) {
    ServerTask server(std::stoi(argv[1]));
    server.run();
}