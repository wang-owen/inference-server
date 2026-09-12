#include "netserver/event_loop.h"

#include <atomic>
#include <fcntl.h>
#include <memory>
#include <system_error>
#include <unistd.h>

#include <cassert>

#include "netserver/channel.h"
#include "netserver/poller.h"

namespace netserver {

EventLoop::EventLoop() : poller_{Poller::create_default()} {
  const auto set_nonblocking = [](int fd) {
    int flags = ::fcntl(fd, F_GETFL);
    if (flags == -1 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK)) {
      throw std::system_error(errno, std::generic_category(), "::fcntl");
    }
  };

  int fildes[2];
  if (::pipe(fildes) == -1) {
    throw std::system_error(errno, std::generic_category(), "::pipe");
  }

  set_nonblocking(fildes[0]);
  set_nonblocking(fildes[1]);
  wakeup_read_fd_ = fildes[0];
  wakeup_write_fd_ = fildes[1];

  wakeup_channel_ = std::make_unique<Channel>(this, wakeup_read_fd_);
  wakeup_channel_->enable_reading();
  wakeup_channel_->set_read_callback([this] { handle_wakeup_read(); });
  update_channel(wakeup_channel_.get());
}

EventLoop::~EventLoop() {
  if (wakeup_channel_ != nullptr) {
    remove_channel(wakeup_channel_.get());
    wakeup_channel_.reset();
    ::close(wakeup_read_fd_);
    ::close(wakeup_write_fd_);
  }
}

void EventLoop::run() {
  while (!quit_.load(std::memory_order_acquire)) {
    active_channels_.clear();
    if (poller_->poll(1000, &active_channels_) == -1) {
      throw std::system_error(errno, std::generic_category(), "::poll");
    }
    for (Channel *channel : active_channels_) {
      channel->handle_event();
    }
  }
}

void EventLoop::stop() {
  quit_.store(true, std::memory_order_release);
  wakeup();
}

void EventLoop::update_channel(Channel *channel) {
  assert(channel != nullptr);
  channels_[channel->fd()] = channel;
  poller_->update_channel(channel);
}

void EventLoop::remove_channel(Channel *channel) {
  assert(channel != nullptr);
  channels_.erase(channel->fd());
  poller_->remove_channel(channel);
}

void EventLoop::wakeup() {
  char byte = 1;
  ::write(wakeup_write_fd_, &byte, 1);
}

void EventLoop::handle_wakeup_read() {
  char buffer[1];
  while (::read(wakeup_read_fd_, buffer, sizeof(buffer)) > 0)
    ;
}

} // namespace netserver
