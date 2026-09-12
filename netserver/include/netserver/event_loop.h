#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

namespace netserver {

class Channel;
class Poller;

// Owns a thread: run() loops on poller_->poll() and, for each Channel* that
// comes back ready, invokes its callbacks. Also owns the fd->Channel*
// bookkeeping behind update_channel()/remove_channel(), and a self-pipe
// wakeup so stop() can be called from another thread and still interrupt a
// blocked poll() promptly.
class EventLoop {
public:
  EventLoop();
  ~EventLoop();
  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;

  // Blocks the calling thread, dispatching channel callbacks until stop()
  // is called.
  void run();

  // Thread-safe. Causes the current or next run() call to return.
  void stop();

  // Registers channel's current interest with the poller, or updates it if
  // already registered. Called by Channel::enable_reading() etc. Must be
  // called from the loop thread.
  void update_channel(Channel *channel);
  void remove_channel(Channel *channel);

private:
  void wakeup();
  void handle_wakeup_read();

  std::unique_ptr<Poller> poller_;
  std::atomic<bool> quit_{false};

  int wakeup_read_fd_ = -1;
  int wakeup_write_fd_ = -1;
  std::unique_ptr<Channel> wakeup_channel_;

  std::unordered_map<int, Channel *> channels_;
  std::vector<Channel *> active_channels_;
};

} // namespace netserver
