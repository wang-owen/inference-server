#pragma once

#include "netserver/tcp_listener.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace netserver {

class Channel;
class EventLoop;

class Acceptor {
public:
  using NewConnectionCallback = std::function<void(int client_fd)>;

  Acceptor(EventLoop *loop, std::string bind_address, std::uint16_t port);
  ~Acceptor();
  Acceptor(const Acceptor &) = delete;
  Acceptor &operator=(const Acceptor &) = delete;

  void set_new_connection_callback(NewConnectionCallback cb) {
    new_connection_callback_ = std::move(cb);
  }

  // Binds, starts listening, and registers the listening fd with the loop.
  // Returns false on failure.
  bool start();

  int listen_fd() const { return listener_.listen_fd(); }

private:
  void handle_read();

  EventLoop *loop_;
  TcpListener listener_;
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback new_connection_callback_;
};

} // namespace netserver
