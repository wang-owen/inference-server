#include "netserver/tcp_listener.h"

#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ConstructsAndDestructsCleanly", "[tcp_listener]") {
  netserver::TcpListener listener("127.0.0.1", 0);
  CHECK(listener.listen_fd() == -1);
}

TEST_CASE("StartSucceedsAndBinds", "[tcp_listener]") {
  netserver::TcpListener listener("127.0.0.1", 0);
  REQUIRE(listener.start());
  CHECK(listener.listen_fd() != -1);

  struct sockaddr address{};
  socklen_t address_len = sizeof(address);
  CHECK(::getsockname(listener.listen_fd(), &address, &address_len) != -1);

  listener.stop();
}

TEST_CASE("StopClosesFdAndIsIdempotent", "[tcp_listener]") {
  netserver::TcpListener listener("127.0.0.1", 0);
  REQUIRE(listener.start());

  listener.stop();
  CHECK(listener.listen_fd() == -1);

  listener.stop();
  CHECK(listener.listen_fd() == -1);
}

TEST_CASE("DestructorClosesFdWithoutExplicitStop", "[tcp_listener]") {
  int fd;
  {
    netserver::TcpListener listener("127.0.0.1", 0);
    REQUIRE(listener.start());
    fd = listener.listen_fd();
  }
  // fd should now be closed; using it should fail with EBADF.
  CHECK(::fcntl(fd, F_GETFD) == -1);
  CHECK(errno == EBADF);
}

TEST_CASE("StartFailsWhenAlreadyListening", "[tcp_listener]") {
  netserver::TcpListener listener("127.0.0.1", 0);
  REQUIRE(listener.start());
  int first_fd = listener.listen_fd();

  CHECK_FALSE(listener.start());
  CHECK(listener.listen_fd() == first_fd);

  listener.stop();
}

TEST_CASE("AcceptsARawConnection", "[tcp_listener]") {
  netserver::TcpListener listener("127.0.0.1", 0);
  REQUIRE(listener.start());

  struct sockaddr_in address{};
  socklen_t address_len = sizeof(address);
  REQUIRE(::getsockname(listener.listen_fd(),
                        reinterpret_cast<struct sockaddr *>(&address),
                        &address_len) != -1);

  int client_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  REQUIRE(client_fd != -1);
  REQUIRE(::connect(client_fd, reinterpret_cast<struct sockaddr *>(&address),
                    address_len) != -1);

  int accepted_fd = ::accept(listener.listen_fd(), nullptr, nullptr);
  CHECK(accepted_fd != -1);

  ::close(client_fd);
  if (accepted_fd != -1) {
    ::close(accepted_fd);
  }
  listener.stop();
}
