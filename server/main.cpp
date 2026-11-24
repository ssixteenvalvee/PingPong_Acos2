#include <server/server.hpp>
#include <sys/socket.h>
#include <iostream>
#include <cerrno>
#include <netinet/in.h>
#include <thread>
#include <vector>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string>
#include <history/history.hpp>
#include <unistd.h>

using namespace std;

int main() {
    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // af_inet = ipv4, sock_stream подразумевает tcp, а аргументом для protocol может быть 0 в таком случае
    if (server < 0) {
        std::cerr << "Error socket init\n";
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(8080); // htons - host to network short. Для перевода порта в Порядок Байтов в Сети.
    // sin_family, sin_port, sin_addr, sin_zero = семейство адрессов (ипв4/6), порт, ип-аддр. клиента, дополнение до размера структуры сок_аддр
    address.sin_addr.s_addr = htonl(INADDR_ANY); //IP-address from anywhere

    int bind_res = bind(server, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    if (bind_res == 0) {
        cerr << "Error: bind\n";
        close(server);
        return 1;
    }

    close(server);
    return 0;
}