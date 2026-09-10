#include "netserver/netserver.h"

#include <cstdio>

int main() {
  netserver::TcpServer server("127.0.0.1", 9090);
  if (!server.start()) {
    std::fprintf(stderr, "netserver: failed to start\n");
    return 1;
  }
  std::printf("netserver: listening on 127.0.0.1:9090 (fd=%d)\n",
              server.listen_fd());
  int client_fd = server.accept_connection();
  std::printf("netserver: accepted connection (fd=%d)\n", client_fd);
  return 0;
}
