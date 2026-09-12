#include "minicache/minicache.h"
#include "netserver/netserver.h"

#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <mutex>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {

std::atomic<netserver::EventLoop *> gLoop{nullptr};

// On some platforms (e.g. macOS/BSD) an accepted socket inherits O_NONBLOCK
// from the listening socket, which Acceptor sets for its own accept loop.
// handle_client() below does blocking read()/send() calls, so each accepted
// fd needs blocking mode restored explicitly.
void set_blocking(int fd) {
  int flags = ::fcntl(fd, F_GETFL);
  ::fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

void handle_shutdown_signal(int) {
  if (netserver::EventLoop *loop = gLoop.load()) {
    loop->stop();
  }
}

// dispatcher (and the LruCache behind it) isn't internally synchronized, and
// every accepted connection now runs handle_client() on its own thread, so
// dispatch calls across connections must be serialized here.
void handle_client(int client_fd, minicache::CommandDispatcher &dispatcher,
                   std::mutex &dispatcher_mutex) {
  auto execute = [&](const std::string &command) {
    std::string response;
    {
      std::lock_guard<std::mutex> lock(dispatcher_mutex);
      response = dispatcher.dispatch(command);
    }
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
  std::mutex dispatcher_mutex;

  netserver::EventLoop loop;
  netserver::Acceptor acceptor(&loop, kBindAddress, kPort);
  acceptor.set_new_connection_callback([&](int client_fd) {
    set_blocking(client_fd);
    std::thread(handle_client, client_fd, std::ref(dispatcher),
                std::ref(dispatcher_mutex))
        .detach();
  });

  if (!acceptor.start()) {
    std::cerr << std::format("Failed to start server at {}:{}\n", kBindAddress,
                             kPort);
    return 1;
  }

  gLoop.store(&loop);
  std::signal(SIGINT, handle_shutdown_signal);
  std::cout << std::format("minicache: listening on {}:{}\n", kBindAddress,
                           kPort);
  loop.run();
}
