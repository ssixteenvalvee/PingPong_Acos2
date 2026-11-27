#include "server.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <string.h>
#include <chrono>
#include <csignal>
#include <atomic>

#define BUFFER_SIZE 256
#define SOC_SAVE_OPT 0

using std::string;

Server::Server() {
    // this->listen_socket; // fd = socket(...) / TCP_Init (STATEMENT HAS NO EFFECT)
    this->port = PORT;
    this->client_socket = Socket(-1);
}

Server::Server(int ownPort) { // perhaps, I'll remove it. maybe setting custom port is redundant.
    this->listen_socket = Socket();
    this->port = ownPort;
}

void Server::setPort(int newPort) {
    this->port = newPort;
}

bool Server::init() {
    int bind_res = -1;
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(this->port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int option = 1;
    setsockopt(this->listen_socket.getFd(), SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)); // re-use option. Otherwise the same socket will be closed for 2 minutes.
    if (this->listen_socket.isOpen()) {
        bind_res = bind(this->listen_socket.getFd(), reinterpret_cast<const sockaddr*>(&address), sizeof(address));
    } else {
        std::cerr << "Socket is not opened" << std::endl;
        return 0;
    }
    if (bind_res < 0) {
        std::cerr << "[SERV] Bind error" << std::endl;
        return 0;
    }
    if (listen(this->listen_socket.getFd(), 1) < 0) {
        std::cerr << "[SERV] listen error" << std::endl;
        return 0;
    }
    std::cout << "[SERV] Init complete" << std::endl;
    return 1;
}

bool Server::run() {
    if (!this->init()) {
        std::cerr << "[SERV] Can not Init listening socket" << std::endl;
        return 0;
    }
    string message;
    this->is_run = true;
    while (this->is_run == true) {
        bool accepted = this->acceptClient();
        if (accepted != true) {
            std::cerr << "[SERV] Client connection failure" << std::endl;
            return 0;
        }
        while(this->is_run == true && this->client_socket.isOpen()) {
            bool rec_Status = receive(message, this->client_ip, pending);
            if (rec_Status == false) {
                std::cout << "[SERV] Client disconnected." << std::endl;
                this->client_socket.close();
                this->pending.clear();
                break;
            }
            if (!message.empty()) {
                // add history/log saving here
                //auto nowt = std::chrono::system_clock::now();
                //std::chrono::system_clock::to_time_t(nowt);
                std::cout << "[SERV] MSG: " << message << std::endl; 
            } else {
                std::cout << "Got nothing" << std::endl; 
            }
            if (message == "ping") {
                this->send("pong"); // '\0' auto-add inside send
            } else if (message == "/shutdown") {
                this->send("Ok then. Bye!");
                this->is_run = false;
            } else {
                this->send("what?");
            }
        }
    }
    this->is_run = false;
    std::cout << "[SERV] Run stop" << std::endl;
    this->pending.clear();
    return true;
}

bool Server::acceptClient() {
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int cl_Fd = accept(this->listen_socket.getFd(), reinterpret_cast<sockaddr*>(&client_addr), &client_len);
    if (cl_Fd < 0) {
        // std::cerr << "[SERV] Client connection failure =(" << std::endl;
        return false;
    }
    char ip_buffer[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_buffer, sizeof(ip_buffer));
    this->client_ip = ip_buffer;
    this->client_port = ntohs(client_addr.sin_port);
    this->client_socket = Socket(cl_Fd);
    std::cout << "[SERV] Client accepted" << std::endl;
    return 1;
}

bool Server::receive(string& message, const string& clientIP, string& pending) {
    while(true) {
        ssize_t separator = pending.find('\0');
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
        ssize_t bytes_recieved = recv(this->client_socket.getFd(), &buffer, BUFFER_SIZE, SOC_SAVE_OPT); // opt is defined
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

bool Server::send(const string message) {
    if (!this->client_socket.isOpen()) {
        std::cerr << "[SERV] Sending error: No client connected" << std::endl;
        return 0;
    }
    string wholeInfo = message + '\0';
    size_t wholeInfo_Size = wholeInfo.size();
    size_t totalSent = 0;
    while (totalSent < wholeInfo_Size) {
        ssize_t sent = ::send(this->client_socket.getFd(), wholeInfo.data() + totalSent, wholeInfo_Size - totalSent, 0);
        if (sent < 0) {
            if (errno == EINTR) continue;
            std::cerr << "[SERV] Sending error: " << strerror(errno) << std::endl;
            return 0;
        }
        totalSent += sent;
    }
    std::cout << "[SERV] Success: Data send." << std::endl;
    return true;
}

Server::~Server() {
    client_socket.close();
    listen_socket.close();
}