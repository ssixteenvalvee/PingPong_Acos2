#include <sys/socket.h>
#include "socket.hpp"
#include <iostream>
#include <unistd.h>

using std::cout, std::endl;

Socket::Socket() : fd(-1) {}

int Socket::getFd() const {
    return this->fd;
}

Socket::Socket(Socket&& newOne) noexcept : fd(newOne.fd) { // otherwise other socket will stay opened.
    newOne.fd = -1;
}

Socket::Socket(int fd) : fd(fd) {}

Socket& Socket::operator=(Socket&& newOne) noexcept {
    if (this != &newOne) {
        this->close();
        this->fd = newOne.fd;
        newOne.fd = -1;
    }
    return *this;
}


bool Socket::isOpen() const {
    if (this->fd != -1) return true;
    return false;
}

bool Socket::TCP_Init() {
    if (this->isOpen()) {
        cout << "[SCK] Socket is already opened" << endl;
        return false;
    }
    this->fd = socket(AF_INET, SOCK_STREAM, 0);
    return this->isOpen();
}

void Socket::close() {
    if (this->isOpen()) {
        ::close(this->fd);
        cout << "[SCK] file descriptor/socket have been closed" << endl;
        this->fd = -1;
    }
}

Socket::~Socket() {
    this->close();
}