#include "inference_srv/inference_srv.h"

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <exception>
#include <format>
#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "netserver/fd_util.h"
#include "netserver/netserver.h"

namespace {

constexpr std::size_t kBufferSize = 1024;
constexpr std::size_t kMaxPendingSize = 64 * 1024;

// Closes the wrapped descriptor on every exit path out of handle_client.
class FdGuard {
public:
  explicit FdGuard(int fd) : fd_{fd} {}
  ~FdGuard() {
    if (fd_ != -1) {
      ::close(fd_);
    }
  }
  FdGuard(const FdGuard &) = delete;
  FdGuard &operator=(const FdGuard &) = delete;

private:
  int fd_;
};

bool send_message(int client_fd, const std::string &message) {
  std::size_t total_written = 0;
  while (total_written < message.size()) {
    ssize_t bytes_written = ::write(client_fd, message.data() + total_written,
                                    message.size() - total_written);
    if (bytes_written > 0) {
      total_written += static_cast<std::size_t>(bytes_written);
      continue;
    }
    if (bytes_written == -1 && errno == EINTR) {
      continue;
    }
    std::cerr << std::format("Failed to write response to client fd {}: {}\n",
                             client_fd, std::strerror(errno));
    return false;
  }
  return true;
}

ssize_t read_some(int client_fd, char *buf, std::size_t len) {
  while (true) {
    ssize_t n = ::read(client_fd, buf, len);
    if (n >= 0) {
      return n;
    }
    if (errno == EINTR) {
      continue;
    }
    std::cerr << std::format("Failed to read from client fd {}: {}\n",
                             client_fd, std::strerror(errno));
    return -1;
  }
}

void handle_client(int client_fd, inference_srv::BatchingQueue &queue) {
  FdGuard guard{client_fd};

  auto execute = [&queue](std::string command)
      -> std::variant<inference_srv::Response, std::string> {
    std::string token;
    std::vector<std::string> tokens;
    std::istringstream iss{std::move(command)};
    while (iss >> token) {
      tokens.push_back(std::move(token));
    }
    if (tokens.empty()) {
      std::string msg = "Empty input";
      std::cerr << msg << '\n';
      return msg;
    }
    if (tokens.front() != "INFER" || tokens.size() < 3) {
      std::string msg = "Invalid command";
      std::cerr << msg << '\n';
      return msg;
    }

    inference_srv::Request request;
    try {
      request.id = queue.get_id();
      request.client_id = std::stoull(tokens[1]);
      for (std::size_t i = 2; i < tokens.size(); ++i) {
        request.input.push_back(std::stof(tokens[i]));
      }
    } catch (const std::exception &e) {
      // stoull/stof throw on non-numbers
      std::string msg = std::format("Malformed request: {}", e.what());
      std::cerr << msg << '\n';
      return msg;
    }

    std::promise<inference_srv::Response> prom;
    std::future<inference_srv::Response> fut = prom.get_future();
    queue.submit(std::move(request), [&prom](inference_srv::Response response) {
      prom.set_value(std::move(response));
    });

    return fut.get();
  };

  std::array<char, kBufferSize> buffer;
  std::string pending;
  ssize_t bytes_read;
  while ((bytes_read = read_some(client_fd, buffer.data(), buffer.size())) >
         0) {
    pending.append(buffer.data(), static_cast<std::size_t>(bytes_read));

    std::size_t newline_pos;
    // INFER <id> <f1> <f2> ... <fn>\n
    while ((newline_pos = pending.find('\n')) != std::string::npos) {
      std::string command = pending.substr(0, newline_pos);
      pending.erase(0, newline_pos + 1);

      std::variant<inference_srv::Response, std::string> response_or =
          execute(std::move(command));

      if (std::string *ptr = std::get_if<std::string>(&response_or)) {
        std::string message = std::format("ERR {}\n", *ptr);
        if (!send_message(client_fd, message)) {
          return;
        }
        continue;
      }

      inference_srv::Response response =
          std::get<inference_srv::Response>(response_or);

      // Write response back to client
      std::string message = std::format("OK {}", response.client_id);
      for (float f : response.output) {
        message += std::format(" {}", f);
      }
      message += '\n';
      if (!send_message(client_fd, message)) {
        return;
      }
    }

    if (pending.size() > kMaxPendingSize) {
      std::cerr << std::format("Client fd {} exceeded max pending size.\n",
                               client_fd);
      return;
    }
  }
}

} // namespace

int main() {
  static constexpr std::string kBindAddress = "127.0.0.1";
  static constexpr std::uint16_t kPort = 9091;
  static constexpr std::size_t kMaxBatchSize = 4;

  // Writing to a peer that has already hung up would otherwise raise SIGPIPE,
  // whose default disposition kills the server.
  std::signal(SIGPIPE, SIG_IGN);

  inference_srv::InferenceEngine engine;
  inference_srv::BatchingQueue batching_queue(
      kMaxBatchSize, std::chrono::milliseconds(50),
      [&engine](const std::vector<inference_srv::Request> &batch,
                std::vector<inference_srv::Response> &out) {
        engine.run_batch(batch, out);
      });

  netserver::EventLoop event_loop;
  netserver::Acceptor acceptor(&event_loop, kBindAddress, kPort);
  acceptor.set_new_connection_callback([&](int client_fd) {
    try {
      netserver::fd_util::set_blocking(client_fd);
      std::thread(handle_client, client_fd, std::ref(batching_queue)).detach();
    } catch (const std::exception &e) {
      // set_blocking throws std::system_error and the thread constructor throws
      // once the process runs out of threads; this callback runs inside the
      // event loop, so neither may escape.
      std::cerr << std::format("Dropping connection on fd {}: {}\n", client_fd,
                               e.what());
      ::close(client_fd);
    }
  });

  if (!acceptor.start()) {
    std::cerr << std::format("Failed to listen on {}:{}.\n", kBindAddress,
                             kPort);
    return 1;
  }
  event_loop.run();
}
