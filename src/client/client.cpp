#include "client.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <string>
#include <cerrno>
#include <poll.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

Client::Client() {}
Client::~Client() {
    this->cl.close();
}

void Client::setIp(string IP) {
    this->sip = IP;
}
void Client::setPort(int PORT) {
    this->sport = PORT;
}

bool Client::connectTo(const std::string& serverIp, int serverPort) {
    if (!this->cl.isOpen()) {
        std::cerr << "[Client] Error: Client's socket is closed" << std::endl;
        return false;
    }
    sockaddr_in connect_addr{};
    connect_addr.sin_family = AF_INET;
    connect_addr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIp.c_str(), &connect_addr.sin_addr) <= 0) { // c_str from string to char array w/ '\0' (!!!)
        std::cerr << "[CL] Invalid IP address: " << serverIp << std::endl;
        return false;
    }
    if (::connect(this->cl.getFd(), reinterpret_cast<sockaddr*>(&connect_addr), sizeof(connect_addr)) < 0) {
        std::cerr << "[CL] Connection error: " << strerror(errno) << std::endl;
        return false;
    }
    std::cout << "[CL] Connected to " << serverIp << ":" << serverPort << std::endl;
    this->setIp(serverIp);
    this->setPort(serverPort);
    return true;
}

bool Client::receiveCl(string& message) {
    while (true) {
        size_t separator = pending.find('\0');
        if (separator != string::npos) {
            if (separator > BUFFER_SIZE) {
                std::cerr << "[CL] Message from server" << this->sip << "is too large" << std::endl;
                return false;
            }
            message = pending.substr(0, separator);
            pending.erase(0, separator+1);
            return true;
        }
        if (pending.size() > BUFFER_SIZE) {
            std::cerr << "[CL] Message from server" << this->sip << "is too large" << std::endl;
            return false;
        }

        char buffer[BUFFER_SIZE]{};
        ssize_t bytes_received = ::recv(this->cl.getFd(), &buffer, sizeof(buffer), 0);
        if (bytes_received == 0) {
            std::cout << "[CL] EOF Server disconnected" << std::endl;
            return false;
        } else if (bytes_received < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cerr << "[CL] Server timeout" << std::endl;
                return false;
            }
            std::cerr << "[CL] Byte reading failure: " << strerror(errno) << std::endl;
            return false;
        }
        pending.append(buffer, bytes_received);
    }
}

bool Client::sendCl(const std::string& data) {
    if (!this->cl.isOpen()) {
        std::cerr << "[CL] Sending error: Socket is closed" << std::endl;
        return false;
    }

    std::string wholeInfo = data + '\0';
    size_t wholeInfo_Size = wholeInfo.size();
    size_t totalSent = 0;

    while (totalSent < wholeInfo_Size) {
        ssize_t sent = ::send(this->cl.getFd(), wholeInfo.data() + totalSent, wholeInfo_Size - totalSent, 0);
        if (sent < 0) {
            if (errno == EINTR) continue;
            std::cerr << "[Client] Sending error: " << strerror(errno) << std::endl;
            return false;
        }
        totalSent += sent;
    }
    return true;
}

void Client::run() {
    std::cout << "[CL] Connected. Type 'ping', '/history', '/quit' or anything you like:\n";
    // 2 дескрипора: 0 = клавиатура, 1 = сокет сервера
    pollfd fds[2];
    fds[0] = {STDIN_FILENO, POLLIN, 0}; // STDIN_FILENO == keyboard
    fds[1] = {this->cl.getFd(), POLLIN, 0};

    while (true) {
        int activity = ::poll(fds, 2, -1);
        if (activity < 0) {
            if (errno == EINTR) continue; // system call interrutpion if SIGINT (special interuption flag) is catched. nothing serious
            std::cerr << "[CL] Poll error: " << strerror(errno) << std::endl;
            break;
        }
        // enter hit -> read & send
        if (fds[0].revents & POLLIN) {
            std::string input;
            if (!std::getline(std::cin, input) || input == "/quit") {
                break;
            }
            if (!input.empty()) {
                if (!this->sendCl(input)) {
                    break;
                }
            }
        }
        if (fds[1].revents & (POLLIN | POLLHUP | POLLERR)) {
            std::string reply;
            if (!this->receiveCl(reply)) {
                std::cout << "\n[CL] Server disconnected." << std::endl;
                break;
            }
            std::cout << reply << std::endl;
            while (this->pending.find('\0') != std::string::npos) {
                if (this->receiveCl(reply)) {
                    std::cout << reply << std::endl;
                }
            }
        }
    }
}
