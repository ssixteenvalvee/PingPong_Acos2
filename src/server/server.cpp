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
#include <poll.h>
#include <vector>

#define BUFFER_SIZE 4096
#define SOC_SAVE_OPT 0

using std::string, std::vector;

Server::Server() {
    // this->listen_socket; // fd = socket(...) / TCP_Init (STATEMENT HAS NO EFFECT)
    this->port = PORT;
    //this->client_socket = Socket(-1);
}
/*
Server::Server(int ownPort) { // perhaps, I'll remove it. maybe setting custom port is redundant.
    this->listen_socket = Socket();
    this->port = ownPort;
}
*/
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
    this->is_run = true;
    while(this->is_run == true) { // LOOP
        std::vector<pollfd> fds;
        fds.push_back({this->listen_socket.getFd(), POLLIN, 0}); // pollin - data for reading available
        // 0 -> flag for revents -> return events -> and listen_socket does not settle any events so it is 0.
        // linux kernel rewrites revents every time when somebody happens. (!!!). Zero is just a filler I suppose.
        for (const auto& client : this->clients) {
            fds.push_back({client.sock.getFd(), POLLIN, 0});
        }
        // listen_socket is first every time for convinience.

        int activity = ::poll(fds.data(), fds.size(), -1); // -1 == wait for ETERNITY.
        if (activity < 0) {
            std::cerr << "[SERV] Poll error: " << strerror(errno) << std::endl;
            break;
        }
        int clients_exist = static_cast<int>(this->clients.size());
        if (fds[0].revents & POLLIN) { 
            this->acceptClient();
        }
        for (int i = clients_exist - 1; i >=0; i--) {
            short events_returned = fds[i+1].revents;
            if (events_returned & (POLLIN | POLLHUP | POLLERR)) { // POLLHUP - Hang up - повесить трубку
                string message;
                if (!this->receive(this->clients[i], message)) {
                    std::cout << "[SERV] Client " << this->clients[i].ip << " disconnected" << std::endl;
                    this->clients.erase(this->clients.begin() + i);
                    continue;
                }

                if (message == "ping") {
                    this->send(this->clients[i], "pong");
                    this->history.addToLogs("Server", "pong");
                } else if (message == "/shutdown") {
                    this->send(this->clients[i], "Ok then. Bye!");
                    this->history.addToLogs("Server", "Ok then. Bye!");
                    this->is_run = false;
                } else if (message == "/history") {
                    this->send(this->clients[i], history.readLogs());
                } else if (message == "sup, machine!") {
                    this->send(this->clients[i], "ay! wasup! (づ｡◕‿‿◕｡)づ");
                    this->history.addToLogs("Server", "ay! wasup! (づ｡◕‿‿◕｡)づ");
                } else {
                    // this->send(this->clients[i], "what?");
                    this->broadcast("[" + this->clients[i].ip + "]: " + message, i);
                    string author = this->clients[i].ip + std::to_string(this->clients[i].port);
                    this->history.addToLogs(author, message);
                }
            }
        }
    }
    return true;
}

void Server::broadcast(const std::string& message, int sender) {
    for (int i = 0; i < static_cast<int>(this->clients.size()); ++i) {
        if (i != sender) {
            this->send(this->clients[i], message);
        }
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
    char ip_buffer[INET_ADDRSTRLEN]{};
    ClientSession session;
    session.sock = Socket(cl_Fd);
    if (inet_ntop(AF_INET, &client_addr.sin_addr, ip_buffer, sizeof(ip_buffer)) != nullptr) {
        session.ip = ip_buffer;
    }
    session.port = ntohs(client_addr.sin_port); // network to host short
    this->clients.push_back(std::move(session)); // can not copy
    std::cout << "[SERV] Client accepted: " << session.ip << ": " << session.port << std::endl;
    return 1;
}

bool Server::receive(ClientSession& session, string& message) {
    while(true) {
        string& pending = session.pending;
        Socket& client_socket = session.sock;
        string& clientIP = session.ip;
        ssize_t separator = pending.find('\0');
        if (separator != string::npos) {
            if (separator > BUFFER_SIZE) {
                std::cerr << "Message from " << session.ip << " is too large" << std::endl;
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
        ssize_t bytes_recieved = recv(client_socket.getFd(), &buffer, BUFFER_SIZE, SOC_SAVE_OPT); // opt is defined
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

bool Server::send(ClientSession& session, const string message) {
    Socket& client_socket = session.sock;

    if (!client_socket.isOpen()) {
        std::cerr << "[SERV] Sending error: No client connected" << std::endl;
        return 0;
    }
    string wholeInfo = message + '\0';
    size_t wholeInfo_Size = wholeInfo.size();
    size_t totalSent = 0;
    while (totalSent < wholeInfo_Size) {
        ssize_t sent = ::send(client_socket.getFd(), wholeInfo.data() + totalSent, wholeInfo_Size - totalSent, 0);
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
    for (auto& client : clients) {
        client.sock.close();
    }
    listen_socket.close();
}