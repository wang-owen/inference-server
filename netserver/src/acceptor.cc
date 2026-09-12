#include "netserver/acceptor.h"

#include <cerrno>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <thread>

#include "netserver/channel.h"
#include "netserver/event_loop.h"

namespace netserver {

namespace {

void set_nonblocking(int fd) {
  int flags = ::fcntl(fd, F_GETFL);
  ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

} // namespace

Acceptor::Acceptor(EventLoop *loop, std::string bind_address,
                   std::uint16_t port)
    : loop_(loop), listener_{std::move(bind_address), port} {}

Acceptor::~Acceptor() {
  if (channel_) {
    loop_->remove_channel(channel_.get());
  }
  listener_.stop();
}

bool Acceptor::start() {
  if (!listener_.start()) {
    return false;
  }
  // The read callback below relies on accept() returning EAGAIN to know
  // when it has drained every pending connection; that only happens on a
  // non-blocking fd.
  set_nonblocking(listener_.listen_fd());

  channel_ = std::make_unique<Channel>(loop_, listener_.listen_fd());
  channel_->set_read_callback([this] { handle_read(); });
  channel_->enable_reading();
  return true;
}

void Acceptor::handle_read() {
  while (true) {
    int client_fd = ::accept(listener_.listen_fd(), nullptr, nullptr);

    if (client_fd != -1) {
      new_connection_callback_(client_fd);
      continue;
    }

    switch (errno) {
    case EINTR:
    case ECONNABORTED:
    case EPROTO:
      // The listener itself is still healthy (e.g. the peer reset the
      // connection before accept() completed); just try again.
      continue;
    case EAGAIN:
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:
#endif
      // Nothing left to accept right now; hand control back to the loop.
      return;
    default:
      break;
    }

    // Resource exhaustion (EMFILE, ENFILE, ENOBUFS, ENOMEM): the process or
    // system is out of capacity rather than this one attempt having failed.
    // Back off briefly instead of busy-looping while it clears.
    perror("Error accepting connection; backing off");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

} // namespace netserver
