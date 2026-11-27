#include <iostream>
#include <string>
#include "client.hpp"

int main(int argc, char* argv[]) {
    std::string ip = "127.0.0.1";
    int port = 8080;
    string message = "";
    string pending;
    if (argc >= 2) ip = argv[1];
    if (argc >= 3) port = std::stoi(argv[2]);

    Client client;
    if (!client.connectTo(ip, port)) {
        return 1;
    }
    client.run();
    return 0;
}
