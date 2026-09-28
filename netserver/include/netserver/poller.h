#pragma once

#include <memory>
#include <vector>

namespace netserver {

class Channel;

// Abstract interface hiding the underlying event-notification mechanism.
class Poller {
public:
  using ChannelList = std::vector<Channel *>;

  virtual ~Poller() = default;

  // Waits up to timeout_ms for activity, appending ready channels to
  // *active. Returns the number of ready channels, or -1 on error.
  virtual int poll(int timeout_ms, ChannelList *active) = 0;

  // Adds channel to the watch set, or updates its interest if it is already
  // registered.
  virtual void update_channel(Channel *channel) = 0;

  // Removes channel from the watch set. Must be called before the channel
  // is destroyed.
  virtual void remove_channel(Channel *channel) = 0;

  // Returns the default Poller implementation for this platform.
  static std::unique_ptr<Poller> create_default();
};

} // namespace netserver
