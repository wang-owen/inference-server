#include "netserver/acceptor.h"
#include "netserver/event_loop.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>

namespace {

std::uint16_t bound_port(int fd) {
  sockaddr_in addr{};
  socklen_t len = sizeof(addr);
  EXPECT_NE(::getsockname(fd, reinterpret_cast<sockaddr *>(&addr), &len), -1);
  return ntohs(addr.sin_port);
}

} // namespace

// Exercises the whole reactor stack end to end: EventLoop running on its own
// thread, PollPoller watching the Acceptor's listening Channel, a real client
// connecting over a loopback socket, and stop()/wakeup() tearing the loop
// back down afterward.
TEST(AcceptorEventLoopTest, AcceptsConnectionThroughReactor) {
  netserver::EventLoop loop;
  netserver::Acceptor acceptor(&loop, "127.0.0.1", 0);

  std::mutex mu;
  std::condition_variable cv;
  int accepted_fd = -1;

  acceptor.set_new_connection_callback([&](int fd) {
    std::lock_guard<std::mutex> lock(mu);
    accepted_fd = fd;
    cv.notify_one();
  });

  ASSERT_TRUE(acceptor.start());
  std::uint16_t port = bound_port(acceptor.listen_fd());

  std::thread loop_thread([&loop] { loop.run(); });

  int client_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_NE(client_fd, -1);

  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  ASSERT_EQ(::inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr), 1);
  ASSERT_NE(::connect(client_fd, reinterpret_cast<sockaddr *>(&server_addr),
                      sizeof(server_addr)),
            -1);

  {
    std::unique_lock<std::mutex> lock(mu);
    ASSERT_TRUE(cv.wait_for(lock, std::chrono::seconds(2),
                            [&] { return accepted_fd != -1; }));
  }

  sockaddr_in peer{};
  socklen_t peer_len = sizeof(peer);
  EXPECT_NE(::getpeername(accepted_fd, reinterpret_cast<sockaddr *>(&peer),
                          &peer_len),
            -1);

  ::close(client_fd);
  ::close(accepted_fd);

  loop.stop();
  loop_thread.join();
}
