#include "inference_srv/inference_srv.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <vector>

namespace {

double to_ms(std::chrono::steady_clock::duration d) {
  return std::chrono::duration<double, std::milli>(d).count();
}

std::vector<inference_srv::Request> make_requests(int count) {
  std::vector<inference_srv::Request> reqs;
  reqs.reserve(count);
  for (int i = 0; i < count; ++i) {
    reqs.push_back({static_cast<std::uint64_t>(i), {1.0f, 2.0f, 3.0f}});
  }
  return reqs;
}

// Baseline: process one request at a time, each as its own "batch" of 1
double run_naive(const std::vector<inference_srv::Request> &requests) {
  inference_srv::InferenceEngine engine;
  std::vector<inference_srv::Response> responses;

  auto start = std::chrono::steady_clock::now();
  for (const auto &req : requests) {
    std::vector<inference_srv::Request> single{req};
    engine.run_batch(single, responses);
  }
  return to_ms(std::chrono::steady_clock::now() - start);
}

// Batched: submit all requests through a BatchingQueue and wait for every
// response, letting the queue group them by max_batch_size/max_wait
double run_batched(const std::vector<inference_srv::Request> &requests,
                   std::size_t max_batch_size,
                   std::chrono::milliseconds max_wait) {
  inference_srv::InferenceEngine engine;

  std::mutex mtx;
  std::condition_variable cv;
  std::size_t received = 0;

  inference_srv::BatchingQueue queue(
      max_batch_size, max_wait,
      [&engine](const std::vector<inference_srv::Request> &batch,
                std::vector<inference_srv::Response> &out) {
        engine.run_batch(batch, out);
      });

  auto start = std::chrono::steady_clock::now();
  for (const auto &req : requests) {
    queue.submit(req, [&](inference_srv::Response) {
      std::lock_guard<std::mutex> lock(mtx);
      ++received;
      cv.notify_all();
    });
  }

  std::unique_lock<std::mutex> lock(mtx);
  cv.wait(lock, [&] { return received >= requests.size(); });
  return to_ms(std::chrono::steady_clock::now() - start);
}

void run_at_request_count(int count) {
  auto requests = make_requests(count);

  double naive_ms = run_naive(requests);
  double batched_ms = run_batched(requests, /*max_batch_size=*/32,
                                  std::chrono::milliseconds(10));

  std::printf("%6d requests | naive: %8.2f ms | batched: %8.2f ms | speedup: "
              "%.2fx\n",
              count, naive_ms, batched_ms, naive_ms / batched_ms);
}

} // namespace

int main() {
  std::printf("inference_srv throughput: naive (one-at-a-time) vs batched\n");
  for (int count : {10, 50, 200, 1000}) {
    run_at_request_count(count);
  }
  return 0;
}
