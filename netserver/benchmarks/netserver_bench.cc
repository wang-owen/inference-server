#include "netserver/netserver.h"

#include <chrono>
#include <cstdio>

int main() {
    // TODO(milestone-3): once EventLoop is implemented, load-test the real
    // server with `wrk`/`ab`/hundreds of concurrent sockets per the roadmap
    // notes, instead of this synthetic in-process placeholder.
    constexpr int kIterations = 100'000;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        netserver::TcpServer server("127.0.0.1", 0);
        (void)server;
    }
    auto elapsed = std::chrono::steady_clock::now() - start;
    std::printf("TcpServer construct/destruct: %.3f ms for %d iterations\n",
                std::chrono::duration<double, std::milli>(elapsed).count(), kIterations);
    return 0;
}
