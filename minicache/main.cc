#include "minicache/minicache.h"
#include "netserver/netserver.h"

#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

namespace {

std::atomic<netserver::TcpServer *> gServer{nullptr};

void handle_shutdown_signal(int) {
  if (netserver::TcpServer *server = gServer.load()) {
    server->stop();
  }
}

void handle_client(int client_fd, minicache::CommandDispatcher &dispatcher) {
  auto execute = [&](const std::string &command) {
    std::string response = dispatcher.dispatch(command);
    std::size_t sent = 0;
    while (sent < response.size()) {
      ssize_t bytes_sent =
          ::send(client_fd, response.data() + sent, response.size() - sent, 0);
      if (bytes_sent == -1) {
        std::cerr << "Error writing to client.\n";
        return false;
      }
      sent += static_cast<std::size_t>(bytes_sent);
    }
    return true;
  };

  char buffer[1024];
  std::string pending;
  ssize_t bytes_read;
  while ((bytes_read = ::read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
    pending.append(buffer, static_cast<std::size_t>(bytes_read));

    std::size_t newline_pos;
    while ((newline_pos = pending.find('\n')) != std::string::npos) {
      std::string command = pending.substr(0, newline_pos);
      pending.erase(0, newline_pos + 1);
      if (!execute(command)) {
        ::close(client_fd);
        return;
      }
    }
  }
  if (bytes_read == -1) {
    std::cerr << "Error reading from client.\n";
  }

  ::close(client_fd);
}

} // namespace

int main() {
  static constexpr std::string kBindAddress = "127.0.0.1";
  static constexpr std::uint16_t kPort = 9090;

  minicache::LruCache cache(1024);
  minicache::CommandDispatcher dispatcher(cache);
  netserver::TcpServer server{kBindAddress, kPort};
  if (!server.start()) {
    std::cerr << std::format("Failed to start server at {}:{}\n", kBindAddress,
                             kPort);
    return 1;
  }

  gServer.store(&server);
  std::signal(SIGINT, handle_shutdown_signal);
  while (true) {
    int client_fd = server.accept_connection();
    if (client_fd == -1) {
      break;
    }
    handle_client(client_fd, dispatcher);
  }
}
