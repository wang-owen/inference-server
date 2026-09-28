#pragma once

#include <fcntl.h>

#include <system_error>

namespace netserver {

namespace fd_util {

static inline void set_blocking(int fd) {
  int flags = ::fcntl(fd, F_GETFL);
  if (flags == -1 || ::fcntl(fd, F_SETFL, flags & ~O_NONBLOCK)) {
    throw std::system_error(errno, std::generic_category(), "::fcntl");
  }
}

static inline void set_nonblocking(int fd) {
  int flags = ::fcntl(fd, F_GETFL);
  if (flags == -1 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK)) {
    throw std::system_error(errno, std::generic_category(), "::fcntl");
  }
}

} // namespace fd_util

} // namespace netserver
