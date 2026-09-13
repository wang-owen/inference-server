#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace inference_srv {

struct Request {
  std::uint64_t id;
  std::vector<float> input;
};

struct Response {
  std::uint64_t id;
  std::vector<float> output;
};

using BatchHandler = std::function<void(const std::vector<Request> &batch,
                                        std::vector<Response> &out_responses)>;

// Accumulates submitted requests until either `max_batch_size` requests
// have arrived or `max_wait` has elapsed since the first request in the
// current batch, then hands the whole batch to `handler` and routes each
// Response back to its caller via the callback passed to submit().
class BatchingQueue {
public:
  using ResponseCallback = std::function<void(Response)>;

  BatchingQueue(std::size_t max_batch_size, std::chrono::milliseconds max_wait,
                BatchHandler handler);
  ~BatchingQueue();
  BatchingQueue(const BatchingQueue &) = delete;
  BatchingQueue &operator=(const BatchingQueue &) = delete;

  void submit(Request request, ResponseCallback on_response);

private:
  struct PendingRequest {
    Request request;
    ResponseCallback on_response;
  };

  void worker_loop();

  std::size_t max_batch_size_;
  std::chrono::milliseconds max_wait_;
  BatchHandler handler_;

  std::mutex mtx_;
  std::condition_variable cv_;
  std::deque<PendingRequest> pending_;
  bool stop_ = false;
  std::thread worker_thread_;
};

} // namespace inference_srv
