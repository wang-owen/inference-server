#include "netserver/tcp_listener.h"

#include <arpa/inet.h>
#include <cstdio>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace netserver {

TcpListener::TcpListener(std::string bind_address, std::uint16_t port)
    : bind_address_(std::move(bind_address)), port_(port) {}

TcpListener::~TcpListener() { stop(); }

bool TcpListener::start() {
  if (listen_fd_ != -1) {
    fprintf(stderr, "TcpListener::start() called while already listening.\n");
    return false;
  }

  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("Error creating socket endpoint.");
    return false;
  }

  auto fail = [fd](const char *msg) {
    perror(msg);
    if (::close(fd) == -1)
      perror("Error closing socket after setup failure.");
    return false;
  };

  const int opt = 1;
  if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    return fail("Error setting socket options.");

  struct sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port_);
  if (::inet_pton(AF_INET, bind_address_.c_str(), &addr.sin_addr) != 1)
    return fail("Error converting IPv4 address.");

  if (::bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    return fail("Error binding name to socket.");

  if (::listen(fd, kBacklog) == -1)
    return fail("Error listening to socket connections.");

  listen_fd_ = fd;
  return true;
}

void TcpListener::stop() {
  if (listen_fd_ != -1) {
    ::close(listen_fd_);
    listen_fd_ = -1;
  }
}

} // namespace netserver
