#include "netserver/tcp_listener.h"

#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <gtest/gtest.h>

TEST(TcpListenerTest, ConstructsAndDestructsCleanly) {
  netserver::TcpListener listener("127.0.0.1", 0);
  EXPECT_EQ(listener.listen_fd(), -1);
}

TEST(TcpListenerTest, StartSucceedsAndBinds) {
  netserver::TcpListener listener("127.0.0.1", 0);
  ASSERT_TRUE(listener.start());
  EXPECT_NE(listener.listen_fd(), -1);

  struct sockaddr address{};
  socklen_t address_len = sizeof(address);
  EXPECT_NE(::getsockname(listener.listen_fd(), &address, &address_len), -1);

  listener.stop();
}

TEST(TcpListenerTest, StopClosesFdAndIsIdempotent) {
  netserver::TcpListener listener("127.0.0.1", 0);
  ASSERT_TRUE(listener.start());

  listener.stop();
  EXPECT_EQ(listener.listen_fd(), -1);

  listener.stop();
  EXPECT_EQ(listener.listen_fd(), -1);
}

TEST(TcpListenerTest, DestructorClosesFdWithoutExplicitStop) {
  int fd;
  {
    netserver::TcpListener listener("127.0.0.1", 0);
    ASSERT_TRUE(listener.start());
    fd = listener.listen_fd();
  }
  // fd should now be closed; using it should fail with EBADF.
  EXPECT_EQ(::fcntl(fd, F_GETFD), -1);
  EXPECT_EQ(errno, EBADF);
}

TEST(TcpListenerTest, StartFailsWhenAlreadyListening) {
  netserver::TcpListener listener("127.0.0.1", 0);
  ASSERT_TRUE(listener.start());
  int first_fd = listener.listen_fd();

  EXPECT_FALSE(listener.start());
  EXPECT_EQ(listener.listen_fd(), first_fd);

  listener.stop();
}

TEST(TcpListenerTest, AcceptsARawConnection) {
  netserver::TcpListener listener("127.0.0.1", 0);
  ASSERT_TRUE(listener.start());

  struct sockaddr_in address{};
  socklen_t address_len = sizeof(address);
  ASSERT_NE(::getsockname(listener.listen_fd(),
                          reinterpret_cast<struct sockaddr *>(&address),
                          &address_len),
            -1);

  int client_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_NE(client_fd, -1);
  ASSERT_NE(::connect(client_fd, reinterpret_cast<struct sockaddr *>(&address),
                      address_len),
            -1);

  int accepted_fd = ::accept(listener.listen_fd(), nullptr, nullptr);
  EXPECT_NE(accepted_fd, -1);

  ::close(client_fd);
  if (accepted_fd != -1) {
    ::close(accepted_fd);
  }
  listener.stop();
}
