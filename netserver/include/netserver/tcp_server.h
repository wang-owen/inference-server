#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace netserver {

class TcpServer {
public:
  TcpServer(std::string bind_address, std::uint16_t port);
  ~TcpServer();

  TcpServer(const TcpServer &) = delete;
  TcpServer &operator=(const TcpServer &) = delete;

  // Binds and starts listening. Returns false on failure or if already
  // listening.
  bool start();

  // Safe to call from another thread while accept_connection() is blocked:
  // wakes it up so it returns -1 instead of racing on the listening socket.
  // Does not itself close listen_fd_; the thread inside accept_connection()
  // owns that. The caller must join/rendezvous with that thread before
  // destroying this TcpServer.
  void stop();

  // Blocks until a client connects, returning its fd, or -1 if the
  // server was stopped while waiting. Transient accept() failures are retried
  // internally and never surface as a -1 return.
  int accept_connection();

  int listen_fd() const { return listen_fd_; }

private:
  static constexpr int kBacklog = 128;

  void close_listener();

  std::string bind_address_;
  std::uint16_t port_;
  int listen_fd_ = -1;
  int wake_read_fd_ = -1;
  int wake_write_fd_ = -1;
  std::atomic<bool> stop_requested_ = false;
  std::atomic<bool> running_ = false;
};

} // namespace netserver
