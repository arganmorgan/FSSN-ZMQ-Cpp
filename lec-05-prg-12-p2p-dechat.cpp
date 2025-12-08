#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <ctime>

#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

bool global_flag_shutdown = false;

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == delim) {
            out.push_back(cur);
            cur.clear();
        }
        else {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
    return out;
}

std::string search_nameserver(const std::string& ip_mask, const std::string& local_ip_addr, int port_nameserver) {
    zmq::context_t context(1);
    zmq::socket_t req(context, zmq::socket_type::sub);
    for (int last = 1; last < 255; last++) {
        std::string target_ip_addr = "tcp://" + ip_mask + "." + std::to_string(last) + ":" + std::to_string(port_nameserver);
        if (target_ip_addr != local_ip_addr || target_ip_addr == local_ip_addr)
            req.connect(target_ip_addr);
        req.set(zmq::sockopt::rcvtimeo, 2000);
        req.set(zmq::sockopt::subscribe, "NAMESERVER");
    }
    try {
        zmq::message_t msg;
        req.recv(msg);
        std::string s(static_cast<char*>(msg.data()), msg.size());
        auto res_list = split(s, ':');
        if (res_list[0] == "NAMESERVER") {
            return res_list[1];
        } else {
            return "";
        }
    } catch (...) {
        return "";
    }
}

void beacon_nameserver(const std::string& local_ip_addr, int port_nameserver) {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::pub);
    socket.bind("tcp://" + local_ip_addr + ":" + std::to_string(port_nameserver));
    std::cout << "local p2p name server bind to tcp://" << local_ip_addr << ":" << port_nameserver << ".\n";
    while (true) {
        try {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::string msg = "NAMESERVER:" + local_ip_addr;
            socket.send(zmq::buffer(msg));
        }
        catch (...) {
            break;
        }
    }
}

void user_manager_nameserver(const std::string& local_ip_addr, int port_subscribe) {
    std::vector<std::vector<std::string>> user_db;
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://" + local_ip_addr + ":" + std::to_string(port_subscribe));
    std::cout << "local p2p db server activated at tcp://" << local_ip_addr << ":" << port_subscribe << ".\n";
    while (true) {
        try {
            zmq::message_t msg;
            socket.recv(msg);
            std::string s(static_cast<char*>(msg.data()), msg.size());
            auto user_req = split(s, ':');
            user_db.push_back(user_req);
            std::cout << "user registration '" << user_req[1]<< "' from '" << user_req[0] << "'.\n";
            socket.send(zmq::buffer("ok", 2));
        }
        catch (...) {
            break;
        }
    }
}

void relay_server_nameserver(const std::string& local_ip_addr, int port_chat_publisher, int port_chat_collector) {
    zmq::context_t context(1);
    zmq::socket_t publisher(context, zmq::socket_type::pub);
    zmq::socket_t collector(context, zmq::socket_type::pull);
    publisher.bind("tcp://" + local_ip_addr + ":" + std::to_string(port_chat_publisher));
    collector.bind("tcp://" + local_ip_addr + ":" + std::to_string(port_chat_collector));
    std::cout << "local p2p relay server activated at tcp://" << local_ip_addr << ":" << port_chat_publisher << " & " << port_chat_collector << ".\n";
    while (true) {
        try {
            zmq::message_t msg;
            collector.recv(msg);
            std::string message(static_cast<char*>(msg.data()), msg.size());
            std::cout << "p2p-relay:<==> " << message << std::endl;
            publisher.send(zmq::buffer("RELAY:" + message));
        }
        catch (...) {
            break;
        }
    }
}

std::string get_local_ip() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return "127.0.0.1";
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return "127.0.0.1";
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return "127.0.0.1";
    }

    sockaddr_in local{};
    int len = sizeof(local);
    if (getsockname(sock, reinterpret_cast<sockaddr*>(&local), &len) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return "127.0.0.1";
    }

    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &local.sin_addr, buf, sizeof(buf));

    closesocket(sock);
    WSACleanup();

    return std::string(buf);
}

void main_func(char* argv[]) {
    std::string ip_addr_p2p_server;
    int port_nameserver = 9001;
    int port_chat_publisher = 9002;
    int port_chat_collector = 9003;
    int port_subscribe = 9004;

    std::string user_name = argv[1];
    std::string ip_addr = get_local_ip();
    std::string ip_mask = ip_addr.substr(0, ip_addr.find_last_of('.'));

    std::cout << "searching for p2p server.\n";

    std::string name_server_ip_addr = search_nameserver(ip_mask, ip_addr, port_nameserver);
    if (name_server_ip_addr.empty()) {
        ip_addr_p2p_server = ip_addr;
        std::cout << "p2p server is not found, and p2p server mode is activated.\n";
        std::thread beacon_thread(beacon_nameserver, ip_addr, port_nameserver);
        beacon_thread.detach();
        std::cout << "p2p beacon server is activated.\n";
        std::thread db_thread(user_manager_nameserver, ip_addr, port_subscribe);
        db_thread.detach();
        std::cout << "p2p subsciber database server is activated.\n";
        std::thread relay_thread(relay_server_nameserver, ip_addr, port_chat_publisher, port_chat_collector);
        relay_thread.detach();
        std::cout << "p2p message relay server is activated.\n";
    }
    else {
        ip_addr_p2p_server = name_server_ip_addr;
        std::cout << "p2p server found at " << ip_addr_p2p_server << ", and p2p client mode is activated.\n";
    }

    std::cout << "starting user registration procedure.\n";

    zmq::context_t db_client_context(1);
    zmq::socket_t db_client_socket(db_client_context, zmq::socket_type::req);
    db_client_socket.connect("tcp://" + ip_addr_p2p_server + ":" + std::to_string(port_subscribe));
    db_client_socket.send(zmq::buffer(ip_addr + ":" + user_name));
    zmq::message_t reply;
    db_client_socket.recv(reply);
    std::string reply_str(static_cast<char*>(reply.data()), reply.size());
    if (reply_str == "ok")
        std::cout << "user registration to p2p server completed.\n";
    else
        std::cout << "user registration to p2p server failed.\n";

    std::cout << "starting message transfer procedure.\n";

    zmq::context_t relay_client(1);
    zmq::socket_t p2p_rx(relay_client, zmq::socket_type::sub);
    p2p_rx.set(zmq::sockopt::subscribe, "RELAY");
    p2p_rx.connect("tcp://" + ip_addr_p2p_server + ":" + std::to_string(port_chat_publisher));
    zmq::socket_t p2p_tx(relay_client, zmq::socket_type::push);
    p2p_tx.connect("tcp://" + ip_addr_p2p_server + ":" + std::to_string(port_chat_collector));

    std::cout << "starting autonomous message transmit and receive scenario.\n";

    while (true) {
        zmq::pollitem_t items[] = {
            { static_cast<void*>(p2p_rx), 0, ZMQ_POLLIN, 0 }
        };
        zmq::poll(items, 1, std::chrono::milliseconds(100));
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            p2p_rx.recv(msg);
            std::string message(static_cast<char*>(msg.data()), msg.size());
            auto parts = split(message, ':');
            std::cout << "p2p-recv::<<== " << parts[1] << ":" << parts[2] << std::endl;
        }
        else {
            int r = std::rand() % 100 + 1;
            if (r < 10) {
                std::this_thread::sleep_for(std::chrono::seconds(3));
                std::string msg = "(" + user_name + "," + ip_addr + ":ON)";
                p2p_tx.send(zmq::buffer(msg));
                std::cout << "p2p-send::==>> " << msg << std::endl;
            }
            else if (r > 90) {
                std::this_thread::sleep_for(std::chrono::seconds(3));
                std::string msg = "(" + user_name + "," + ip_addr + ":OFF)";
                p2p_tx.send(zmq::buffer(msg));
                std::cout << "p2p-send::==>> " << msg << std::endl;
            }
        }
    }

    std::cout << "closing p2p chatting program.\n";

    global_flag_shutdown = true;
    db_client_socket.close();
    p2p_rx.close();
    p2p_tx.close();
    db_client_context.close();
    relay_client.close();
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cout << "usage is 'python dechat.py _user-name_'.\n";
    }
    else {
        std::cout << "starting p2p chatting program.\n";
        srand(time(NULL));
        main_func(argv);
    }
}