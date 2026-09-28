#include "inference_srv/inference_srv.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace {

std::vector<inference_srv::Request> make_requests(int count) {
  std::vector<inference_srv::Request> reqs;
  reqs.reserve(count);
  for (int i = 0; i < count; ++i) {
    reqs.push_back({static_cast<std::uint64_t>(i), {1.0f, 2.0f, 3.0f}});
  }
  return reqs;
}

// Baseline: process one request at a time, each as its own "batch" of 1
std::size_t run_naive(inference_srv::InferenceEngine &engine,
                      const std::vector<inference_srv::Request> &requests) {
  std::vector<inference_srv::Response> responses;
  for (const auto &req : requests) {
    std::vector<inference_srv::Request> single{req};
    engine.run_batch(single, responses);
  }
  return responses.size();
}

// Batched: submits requests through one long-lived BatchingQueue and waits
// for every response, letting the queue group them by max_batch_size/max_wait.
// The queue outlives each timed run so its thread startup and shutdown aren't
// measured.
class BatchedRunner {
public:
  BatchedRunner(std::size_t max_batch_size, std::chrono::milliseconds max_wait)
      : queue_(max_batch_size, max_wait,
               [this](const std::vector<inference_srv::Request> &batch,
                      std::vector<inference_srv::Response> &out) {
                 engine_.run_batch(batch, out);
               }) {}

  std::size_t run(const std::vector<inference_srv::Request> &requests) {
    {
      std::lock_guard<std::mutex> lock(mtx_);
      received_ = 0;
    }
    for (const auto &req : requests) {
      queue_.submit(req, [this](inference_srv::Response) {
        std::lock_guard<std::mutex> lock(mtx_);
        ++received_;
        cv_.notify_all();
      });
    }
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [&] { return received_ >= requests.size(); });
    return received_;
  }

private:
  inference_srv::InferenceEngine engine_;
  std::mutex mtx_;
  std::condition_variable cv_;
  std::size_t received_ = 0;
  inference_srv::BatchingQueue queue_;
};

} // namespace

TEST_CASE("inference_srv throughput: naive (one-at-a-time) vs batched",
          "[!benchmark][inference_srv]") {
  const int count = GENERATE(10, 50, 200, 1000);
  const auto requests = make_requests(count);
  const std::string suffix = " | " + std::to_string(count) + " requests";

  inference_srv::InferenceEngine engine;
  BENCHMARK("naive" + suffix) { return run_naive(engine, requests); };

  BatchedRunner batched(/*max_batch_size=*/32, std::chrono::milliseconds(10));
  BENCHMARK("batched" + suffix) { return batched.run(requests); };
}
