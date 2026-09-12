#include "netserver/poller.h"
#include "netserver/poller_poll.h"

namespace netserver {

std::unique_ptr<Poller> Poller::create_default() {
  return std::make_unique<PollPoller>();
}

} // namespace netserver
