#pragma once

class Socket {
private:
    int fd; // file descriptor
public:
    Socket();
    explicit Socket(int fd);

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete; // no copy! race condition or something even worse may occur..
    Socket& operator=(Socket&& newOne) noexcept;

    bool TCP_Init();
    int getFd() const;
    Socket(Socket&& newOne) noexcept;
    bool isOpen() const;
    void close();

    ~Socket();
};