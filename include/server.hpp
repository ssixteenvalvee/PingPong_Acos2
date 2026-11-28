#pragma once

#include "socket.hpp"
#include "history.hpp"
#include <string>
#include <vector>
#include <filesystem>
#define PORT 8080

struct ClientSession {
    Socket sock;
    std::string ip;
    int port = 0;
    std::string pending;
};

class Server {
    private:
        Socket listen_socket;
        std::vector<ClientSession> clients;
        int port = PORT;
        bool is_run = false;
        History history{std::filesystem::current_path()};
    public:
        std::string pending;
        std::string client_ip; // for logging
        int client_port;

        Server();
        explicit Server(int port);

        void broadcast(const std::string& message, int sender);
        void setPort(int newPort);
        bool init();
        bool run();
        bool acceptClient();

        bool send(ClientSession& session, std::string message);
        bool receive(ClientSession& session, std::string& message);

        ~Server();
};