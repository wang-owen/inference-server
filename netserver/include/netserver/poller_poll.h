#pragma once

#include "netserver/poller.h"

#include <poll.h>

#include <unordered_map>
#include <vector>

namespace netserver {

// Poller implementation backed by ::poll().
class PollPoller : public Poller {
public:
  PollPoller() = default;
  ~PollPoller() override = default;
  PollPoller(const PollPoller &) = delete;
  PollPoller &operator=(const PollPoller &) = delete;

  int poll(int timeout_ms, ChannelList *active) override;
  void update_channel(Channel *channel) override;
  void remove_channel(Channel *channel) override;

private:
  std::vector<pollfd> pollfds_;
  std::unordered_map<int, Channel *> channels_;
};

} // namespace netserver
