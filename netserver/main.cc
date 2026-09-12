#include "netserver/netserver.h"

#include <cstdio>

int main() {
  netserver::EventLoop loop;
  netserver::Acceptor acceptor(&loop, "127.0.0.1", 9090);
  acceptor.set_new_connection_callback([](int client_fd) {
    std::printf("netserver: accepted connection (fd=%d)\n", client_fd);
  });

  if (!acceptor.start()) {
    std::fprintf(stderr, "netserver: failed to start\n");
    return 1;
  }
  std::printf("netserver: listening on 127.0.0.1:9090 (fd=%d)\n",
              acceptor.listen_fd());
  loop.run();
  return 0;
}
