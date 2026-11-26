#pragma once
#include "socket.hpp"
#include <string>
#define PORT 8080

class Server {
    private:
        Socket listen_socket;
        Socket client_socket;
        int port = PORT;
    public:
        std::string client_ip; // for logging
        int client_port;

        Server();
        explicit Server(int port);

        void setPort(int newPort);
        void init();
        bool run();
        bool acceptClient();

        bool send(const std::string message);
        bool receive(std::string& message, const std::string& clientIP, std::string& pending);

        ~Server();
};