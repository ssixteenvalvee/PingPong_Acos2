#include "server.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <string.h>
#include <chrono>

#define BUFFER_SIZE 256
#define SOC_SAVE_OPT 0

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
        return;
    }
    if (bind_res < 0) {
        std::cerr << "[SERV] Bind error" << std::endl;
        return;
    }
    if (listen(this->listen_socket.getFd(), 1) < 0) {
        std::cerr << "[SERV] listen error" << std::endl;
        return;
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
        std::cerr << "[SERV] Client connection failure" << std::endl;
        return 0;
    }
    string message;
    string pending;
    bool rec_Status = receive(message, this->client_ip, pending);
    if (!message.empty()) {
        // add history/log saving here
        auto now = std::chrono::system_clock::now;
        std::cout << "[" << now << "] MSG: " << message << std::endl; 
    } else {
        std::cout << "Got nothing" << std::endl; 
    }
}

bool Server::acceptClient() {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int cl_Fd = accept(this->listen_socket.getFd(), reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (cl_Fd < 0) {
        // std::cerr << "[SERV] Client connection failure =(" << std::endl;
        return false;
    }
    this->client_ip = ntohl(client_addr.sin_addr.s_addr); // network to host long
    this->client_port = client_addr.sin_port;
    this->client_socket = Socket(cl_Fd);
    return 1;
}

bool Server::receive(string& message, const string& clientIP, string& pending) {
    while(true) {
        size_t separator = pending.find('\0');
        if (separator != string::npos) {
            if (separator > BUFFER_SIZE) {
                std::cerr << "Message from " << clientIP << " is too large" << std::endl;
                return 0;
            }
            message = pending.substr(0, separator);
            pending.erase(0, separator + 1); // +1 including \0
            return 1;
        }
        if (pending.size() > BUFFER_SIZE) {
            std::cerr << "Message from " << clientIP << " is too large" << std::endl;
            return 0;
        }

        char buffer[BUFFER_SIZE]{};
        size_t bytes_recieved = recv(this->client_socket.getFd(), &buffer, BUFFER_SIZE, SOC_SAVE_OPT); // opt is defined
        if (bytes_recieved == 0) {
            std::cout << "EOF Client disconnected: " << clientIP << std::endl;
            return 0;
        } else if (bytes_recieved == -1) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cerr << "[SERV] Client timeout: " << clientIP << std::endl;
                return false;
            }
            std::cerr << "[SERV] Byte reading failure from " << clientIP << ": " << strerror(errno) << std::endl;
            return 0;
        }
        pending.append(buffer, bytes_recieved);
    }
}

Server::~Server() {
    client_socket.close();
    listen_socket.close();
}