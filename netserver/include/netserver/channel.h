#pragma once

#include <functional>

namespace netserver {

class EventLoop;

// Represents one fd's registration with the event loop: what it's being
// watched for (currently: readable), and the callback to run when that
// interest is satisfied. Does not own the fd: whoever created it is
// responsible for closing it. Holds a back-pointer to its EventLoop so
// enable_reading() can self-register rather than making every owner
// remember to call loop_->update_channel() by hand.
class Channel {
public:
  using EventCallback = std::function<void()>;

  Channel(EventLoop *loop, int fd);
  ~Channel();
  Channel(const Channel &) = delete;
  Channel &operator=(const Channel &) = delete;

  int fd() const { return fd_; }

  // Bitmask of events currently registered; consumed by Poller when it
  // builds its watch set.
  int events() const { return events_; }

  // Set by Poller to the events that actually fired, then handle_event()
  // dispatches to the matching callback.
  void set_revents(int revents) { revents_ = revents; }
  void handle_event();

  void set_read_callback(EventCallback cb) { read_callback_ = std::move(cb); }

  void enable_reading();

private:
  void update();

  EventLoop *loop_;
  int fd_;
  int events_ = 0;
  int revents_ = 0;

  EventCallback read_callback_;
};

} // namespace netserver
