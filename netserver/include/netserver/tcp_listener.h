#pragma once

#include <cstdint>
#include <string>

namespace netserver {

class TcpListener {
public:
  TcpListener(std::string bind_address, std::uint16_t port);
  ~TcpListener();
  TcpListener(const TcpListener &) = delete;
  TcpListener &operator=(const TcpListener &) = delete;

  // Binds and starts listening. Returns false on failure or if already
  // listening.
  bool start();

  // Closes listen_fd_ if open. Idempotent; also called by the destructor,
  // so it's safe to let a TcpListener just go out of scope. TcpListener has
  // no knowledge of loops or threads, so unlike TcpServer::stop() there is
  // no blocking accept() to unblock here.
  void stop();

  int listen_fd() const { return listen_fd_; }

private:
  static constexpr int kBacklog = 128;

  std::string bind_address_;
  std::uint16_t port_;
  int listen_fd_ = -1;
};

} // namespace netserver
