#include "netserver/tcp_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include <gtest/gtest.h>

TEST(TcpServerTest, ConstructsAndDestructsCleanly) {
  netserver::TcpServer server("127.0.0.1", 0);
  EXPECT_EQ(server.listen_fd(), -1);
}

TEST(TcpServerTest, StartSucceedsAndBinds) {
  netserver::TcpServer server("127.0.0.1", 0);
  ASSERT_TRUE(server.start());
  EXPECT_NE(server.listen_fd(), -1);

  std::thread client_thread{[&server]() {
    struct sockaddr address{};
    socklen_t address_len = sizeof(address);
    ASSERT_NE(::getsockname(server.listen_fd(), &address, &address_len), -1);

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_NE(fd, -1);
    EXPECT_NE(::connect(fd, &address, address_len), -1);
    ::close(fd);
  }};

  int client_fd = server.accept_connection();
  EXPECT_NE(client_fd, -1);
  if (client_fd != -1) {
    ::close(client_fd);
  }

  client_thread.join();
  server.stop();
}

TEST(TcpServerTest, StopUnblocksAcceptConnection) {
  netserver::TcpServer server("127.0.0.1", 0);
  ASSERT_TRUE(server.start());
  std::thread blocking_thread{[&server] {
    int response = server.accept_connection();
    EXPECT_EQ(response, -1);
  }};
  server.stop();
  blocking_thread.join();
}
