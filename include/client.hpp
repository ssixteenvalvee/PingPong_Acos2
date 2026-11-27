#pragma once

#include "socket.hpp"
#include <string>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>

using std::string;

class Client {
    private:
        Socket cl;
        string sip;
        int sport;
        string pending;
    public:
        Client();
        ~Client();

        void setIp(string IP);
        void setPort(int PORT);

        bool connectTo(const std::string& serverIp, int serverPort);
        bool receiveCl(string& message);
        bool sendCl(const string& data);
        void run();
};