#include "server.hpp"
#include <csignal>
#include <atomic>

std::atomic<bool> g_stop{false};

void sigint_handler(int) {
    g_stop = true;
}

using namespace std;

int main() {
    struct sigaction sa{};
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa,  nullptr);
    Server server = Server();
    server.run();
    return 0;
}