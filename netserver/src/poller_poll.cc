#include "netserver/poller_poll.h"

#include <algorithm>
#include <cassert>
#include <poll.h>

#include <cerrno>
#include <sys/poll.h>

#include "netserver/channel.h"

namespace netserver {

int PollPoller::poll(int timeout_ms, ChannelList *active) {
  int num_ready;
  do {
    num_ready = ::poll(pollfds_.data(), pollfds_.size(), timeout_ms);
  } while (num_ready == -1 && errno == EINTR);

  if (num_ready == -1) {
    return -1;
  }

  for (const pollfd &fds : pollfds_) {
    if (fds.revents != 0) {
      Channel *channel = channels_.at(fds.fd);
      channel->set_revents(fds.revents);
      active->push_back(channel);
    }
  }

  return num_ready;
}

void PollPoller::update_channel(Channel *channel) {
  int fd = channel->fd();
  if (channels_.contains(fd)) {
    auto it = std::find_if(pollfds_.begin(), pollfds_.end(),
                           [fd](const pollfd &fds) { return fds.fd == fd; });
    it->events = channel->events();
  } else {
    channels_.emplace(fd, channel);
    pollfds_.emplace_back(fd, channel->events());
  }
}

void PollPoller::remove_channel(Channel *channel) {
  assert(channel != nullptr);
  int fd = channel->fd();
  auto channels_it = channels_.find(fd);
  assert(channels_it != channels_.cend());
  channels_.erase(channels_it);
  auto pollfds_it =
      std::find_if(pollfds_.begin(), pollfds_.end(),
                   [fd](const pollfd &fds) { return fds.fd == fd; });
  pollfds_.erase(pollfds_it);
}

} // namespace netserver
