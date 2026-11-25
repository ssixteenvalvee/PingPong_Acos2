#include "server.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>

using std::string;

Server::Server() {
    this->listen_socket = Socket(); // fd = socket(...) / TCP_Init
    this->port = PORT;
}

explicit Server::Server(int ownPort) { // perhaps, I'll remove it. maybe setting custom port is redundant.
    this->listen_socket = Socket();
    this->port = ownPort;
}

void Server::setPort(int newPort) {
    this->port = newPort;
}

void Server::init() {
    int bind_res = -1;
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(this->port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (this->listen_socket.isOpen()) {
        int bind_res = bind(this->listen_socket.getFd(), reinterpret_cast<const sockaddr*>(&address), sizeof(address));
    } else {
        std::cerr << "Socket is not opened" << std::endl;
        exit(-1);
    }
    if (bind_res < 0) {
        std::cerr << "[SERV] Bind error" << std::endl;
        exit(-1);
    }
    if (listen(this->listen_socket.getFd(), 1) < 0) {
        std::cerr << "[SERV] listen error" << std::endl;
    }
    return;
}

bool Server::run() {
    if (!this->listen_socket.TCP_Init()) {
        std::cerr << "[SERV] Can not Init listening socket" << std::endl;
        return -1;
    }
    int option = 1;
    setsockopt(this->listen_socket.getFd(), SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)); // re-use option. Otherwise the same socket will be closed for 2 minutes.
    bool accepted = this->acceptClient();
    if (accepted != true) {
        std::cerr << "[SERV] No client accepted" << std::endl;
        return 0;
    }
}

bool Server::acceptClient() {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int cl_Fd = accept(this->listen_socket.getFd(), reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (cl_Fd < 0) {
        std::cerr << "[SERV] Can not listen =(" << std::endl;
        return false;
    }
    this->client_socket = Socket(cl_Fd);
    return 1;
}

string Server::receive() {
    string result = "";

    return result;
}

Server::~Server() {
    client_socket.close();
    listen_socket.close();
}