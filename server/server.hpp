#pragma once

class Server {
    private:
        int server_fd;
        int client_fd;
    public:
        Server();
        explicit Server(int port);

        int getServerFd();
        void setServerFd();

        int getClientFd();
        void setClientFd();

        void listenMessage();
        void sendMessage();

        ~Server();
};