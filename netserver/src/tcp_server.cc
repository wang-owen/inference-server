#include "netserver/tcp_server.h"

#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace netserver {

namespace {

void set_nonblocking(int fd) {
  int flags = ::fcntl(fd, F_GETFL);
  ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

} // namespace

TcpServer::TcpServer(std::string bind_address, std::uint16_t port)
    : bind_address_(std::move(bind_address)), port_(port) {}

TcpServer::~TcpServer() {
  stop();
  close_listener();
}

bool TcpServer::start() {
  if (listen_fd_ != -1) {
    if (!stop_requested_.load(std::memory_order_acquire)) {
      fprintf(stderr, "TcpServer::start() called while already listening.");
      return false;
    }
    // A prior session was stopped but never reclaimed by accept_connection()
    // (no thread was blocked in it to observe the wakeup). Per stop()'s
    // contract, no thread should still be polling listen_fd_ at this point,
    // so it's safe for start() to close it here and rebind.
    close_listener();
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

  int wake_fds[2];
  if (::pipe(wake_fds) == -1)
    return fail("Error creating wakeup pipe.");
  set_nonblocking(wake_fds[0]);
  set_nonblocking(wake_fds[1]);

  listen_fd_ = fd;
  wake_read_fd_ = wake_fds[0];
  wake_write_fd_ = wake_fds[1];
  stop_requested_.store(false, std::memory_order_relaxed);
  running_.store(true, std::memory_order_release);

  return true;
}

void TcpServer::stop() {
  stop_requested_.store(true, std::memory_order_release);
  if (wake_write_fd_ != -1) {
    char byte = 1;
    // Best-effort: EAGAIN just means a wakeup byte is already pending.
    (void)::write(wake_write_fd_, &byte, 1);
  }
}

void TcpServer::close_listener() {
  if (listen_fd_ != -1) {
    ::close(listen_fd_);
    listen_fd_ = -1;
  }
  if (wake_read_fd_ != -1) {
    ::close(wake_read_fd_);
    wake_read_fd_ = -1;
  }
  if (wake_write_fd_ != -1) {
    ::close(wake_write_fd_);
    wake_write_fd_ = -1;
  }
}

int TcpServer::accept_connection() {
  if (!running_.load(std::memory_order_acquire)) {
    return -1;
  }

  for (;;) {
    if (stop_requested_.load(std::memory_order_acquire)) {
      return -1;
    }

    struct pollfd fds[2];
    fds[0].fd = listen_fd_;
    fds[0].events = POLLIN;
    fds[1].fd = wake_read_fd_;
    fds[1].events = POLLIN;

    int ready;
    do {
      ready = ::poll(fds, 2, -1);
    } while (ready == -1 && errno == EINTR);

    if (ready == -1) {
      perror("Error polling listening socket.");
      close_listener();
      return -1;
    }

    if (fds[1].revents & POLLIN) {
      close_listener();
      return -1;
    }

    if (!(fds[0].revents & POLLIN)) {
      continue;
    }

    int client_fd =
        ::accept(listen_fd_, /*address=*/nullptr, /*address_len=*/nullptr);
    if (client_fd != -1) {
      return client_fd;
    }

    switch (errno) {
    case EINTR:
    case ECONNABORTED:
    case EPROTO:
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
      // The listener itself is still healthy (e.g. the peer reset the
      // connection before accept() completed); just poll again instead of
      // reporting this as a shutdown.
      continue;
    default:
      break;
    }

    // Resource exhaustion (EMFILE, ENFILE, ENOBUFS, ENOMEM): the process or
    // system is out of capacity rather than this one attempt having failed.
    // Back off briefly instead of busy-looping while it clears.
    perror("Error accepting connection; backing off");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

} // namespace netserver
