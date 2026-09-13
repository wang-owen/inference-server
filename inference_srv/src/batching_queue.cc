#include "inference_srv/batching_queue.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace inference_srv {

BatchingQueue::BatchingQueue(std::size_t max_batch_size,
                             std::chrono::milliseconds max_wait,
                             BatchHandler handler)
    : max_batch_size_(max_batch_size), max_wait_(max_wait),
      handler_(std::move(handler)) {
  worker_thread_ = std::thread{[this] { worker_loop(); }};
}

BatchingQueue::~BatchingQueue() {
  {
    std::lock_guard<std::mutex> lock{mtx_};
    stop_ = true;
    cv_.notify_all();
  }
  worker_thread_.join();
}

void BatchingQueue::submit(Request request, ResponseCallback on_response) {
  std::lock_guard<std::mutex> lock{mtx_};
  pending_.emplace_back(std::move(request), std::move(on_response));
  cv_.notify_one();
}

void BatchingQueue::worker_loop() {
  while (true) {
    std::unique_lock<std::mutex> lock{mtx_};
    if (stop_)
      break;
    auto deadline = std::chrono::steady_clock::now() + max_wait_;
    cv_.wait_until(lock, deadline, [this] {
      return stop_ || pending_.size() >= max_batch_size_;
    });

    std::vector<Request> batch;
    std::vector<Response> responses;
    std::size_t len =
        std::min(pending_.size(), static_cast<std::size_t>(max_batch_size_));
    if (len == 0) {
      continue;
    }
    for (std::size_t i = 0; i < len; ++i) {
      batch.push_back(std::move(pending_[i].request));
    }

    handler_(batch, responses);

    std::unordered_map<std::uint64_t, Response &> response_map;
    for (auto &response : responses) {
      response_map.emplace(response.id, response);
    }

    for (std::size_t i = 0; i < len; ++i) {
      pending_[i].on_response(response_map.at(pending_[i].request.id));
    }

    pending_.erase(pending_.begin(), pending_.begin() + len);
  }
}

} // namespace inference_srv
