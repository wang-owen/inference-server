#include "netserver/channel.h"

#include "netserver/event_loop.h"

#include <poll.h>
#include <sys/poll.h>

namespace netserver {

Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd) {}

Channel::~Channel() = default;

void Channel::update() { loop_->update_channel(this); }

void Channel::enable_reading() {
  events_ |= POLLIN;
  update();
}

void Channel::handle_event() {
  if ((revents_ & POLLIN) != 0 && read_callback_) {
    read_callback_();
  }
}

} // namespace netserver
