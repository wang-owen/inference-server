#include "inference_srv/inference_srv.h"

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>

namespace {

class ResponseWaiter {
public:
  explicit ResponseWaiter(std::size_t expected) : expected_(expected) {}

  void on_response(inference_srv::Response response) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::printf("  response id=%llu output[0]=%.1f\n",
                static_cast<unsigned long long>(response.id),
                response.output.empty() ? 0.0f : response.output[0]);
    ++received_;
    cv_.notify_all();
  }

  void wait_for_all() {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [&] { return received_ >= expected_; });
  }

private:
  std::mutex mtx_;
  std::condition_variable cv_;
  std::size_t expected_;
  std::size_t received_ = 0;
};

} // namespace

int main() {
  inference_srv::InferenceEngine engine;
  constexpr std::size_t kMaxBatchSize = 4;
  inference_srv::BatchingQueue queue(
      kMaxBatchSize, std::chrono::milliseconds(50),
      [&engine](const std::vector<inference_srv::Request> &batch,
                std::vector<inference_srv::Response> &out) {
        engine.run_batch(batch, out);
      });

  // Scenario 1: submit exactly kMaxBatchSize requests
  std::printf("submitting a full batch of %zu requests...\n", kMaxBatchSize);
  ResponseWaiter full_batch_waiter(kMaxBatchSize);
  for (std::uint64_t id = 0; id < kMaxBatchSize; ++id) {
    queue.submit(
        inference_srv::Request{.id = id, .input = {static_cast<float>(id)}},
        [&full_batch_waiter](inference_srv::Response response) {
          full_batch_waiter.on_response(std::move(response));
        });
  }
  full_batch_waiter.wait_for_all();

  // Scenario 2: submit fewer than kMaxBatchSize requests
  std::printf("submitting a partial batch of 2 requests (relies on "
              "max_wait)...\n");
  ResponseWaiter partial_batch_waiter(2);
  for (std::uint64_t id = 100; id < 102; ++id) {
    queue.submit(
        inference_srv::Request{.id = id, .input = {static_cast<float>(id)}},
        [&partial_batch_waiter](inference_srv::Response response) {
          partial_batch_waiter.on_response(std::move(response));
        });
  }
  partial_batch_waiter.wait_for_all();

  std::printf("inference_srv demo: all responses received\n");
  return 0;
}
