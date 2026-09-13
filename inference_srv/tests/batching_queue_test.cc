#include "inference_srv/batching_queue.h"

#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>
#include <vector>

using inference_srv::BatchingQueue;
using inference_srv::Request;
using inference_srv::Response;

namespace {

// Thread-safe collector for responses delivered asynchronously from the
// queue's worker thread. Tests block on wait_for_at_least() instead of
// sleeping a fixed amount, since submit() is fire-and-forget.
class ResponseCollector {
public:
  void add(Response response) {
    std::lock_guard<std::mutex> lock(mtx_);
    responses_.push_back(std::move(response));
    cv_.notify_all();
  }

  // Returns false (times out) if `count` responses never arrive within
  // `timeout` -- used to assert something did NOT happen promptly.
  bool wait_for_at_least(std::size_t count, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mtx_);
    return cv_.wait_for(lock, timeout,
                        [&] { return responses_.size() >= count; });
  }

  std::vector<Response> snapshot() {
    std::lock_guard<std::mutex> lock(mtx_);
    return responses_;
  }

private:
  std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<Response> responses_;
};

// Passthrough handler: doubles each input value, like InferenceEngine's
// stub model, without depending on inference_engine.h.
void DoublingHandler(const std::vector<Request> &batch,
                     std::vector<Response> &out_responses) {
  out_responses.clear();
  for (const Request &req : batch) {
    Response resp;
    resp.id = req.id;
    resp.output.resize(req.input.size());
    for (std::size_t i = 0; i < req.input.size(); ++i) {
      resp.output[i] = req.input[i] * 2.0f;
    }
    out_responses.push_back(std::move(resp));
  }
}

} // namespace

TEST(BatchingQueueTest, ConstructsAndDestructsCleanly) {
  BatchingQueue queue(
      8, std::chrono::milliseconds(5),
      [](const std::vector<Request> &, std::vector<Response> &) {});
  SUCCEED();
}

TEST(BatchingQueueTest, FullBatchTriggersBeforeMaxWaitElapses) {
  constexpr std::size_t kBatchSize = 4;
  constexpr auto kMaxWait = std::chrono::milliseconds(500);
  ResponseCollector collector;

  BatchingQueue queue(kBatchSize, kMaxWait, DoublingHandler);
  for (std::uint64_t id = 0; id < kBatchSize; ++id) {
    queue.submit(Request{.id = id, .input = {static_cast<float>(id)}},
                 [&collector](Response r) { collector.add(std::move(r)); });
  }

  // If this only fired via the max_wait timeout, it wouldn't arrive for
  // 500ms -- a generous fraction of that proves it fired on batch size.
  EXPECT_TRUE(
      collector.wait_for_at_least(kBatchSize, std::chrono::milliseconds(100)));
}

TEST(BatchingQueueTest, PartialBatchFlushesAfterMaxWait) {
  constexpr std::size_t kBatchSize = 8;
  constexpr auto kMaxWait = std::chrono::milliseconds(20);
  ResponseCollector collector;

  BatchingQueue queue(kBatchSize, kMaxWait, DoublingHandler);
  queue.submit(Request{.id = 1, .input = {1.0f}},
               [&collector](Response r) { collector.add(std::move(r)); });
  queue.submit(Request{.id = 2, .input = {2.0f}},
               [&collector](Response r) { collector.add(std::move(r)); });

  // Only 2 of the required 8 requests were submitted, so this can only
  // succeed via the max_wait timeout path, not the batch-size path.
  ASSERT_TRUE(collector.wait_for_at_least(2, std::chrono::milliseconds(500)));
  EXPECT_EQ(collector.snapshot().size(), 2u);
}

TEST(BatchingQueueTest, RoutesResponseToCallerMatchingRequestIdNotPosition) {
  constexpr std::size_t kBatchSize = 3;
  constexpr auto kMaxWait = std::chrono::milliseconds(500);
  ResponseCollector collector;

  // Deliberately returns responses in reverse order to prove the queue
  // matches by Response::id rather than assuming out[i] answers batch[i].
  auto reordering_handler = [](const std::vector<Request> &batch,
                               std::vector<Response> &out_responses) {
    out_responses.clear();
    for (auto it = batch.rbegin(); it != batch.rend(); ++it) {
      Response resp;
      resp.id = it->id;
      resp.output = {it->input[0] * 10.0f};
      out_responses.push_back(std::move(resp));
    }
  };

  BatchingQueue queue(kBatchSize, kMaxWait, reordering_handler);
  for (std::uint64_t id = 0; id < kBatchSize; ++id) {
    queue.submit(Request{.id = id, .input = {static_cast<float>(id)}},
                 [&collector](Response r) { collector.add(std::move(r)); });
  }

  ASSERT_TRUE(
      collector.wait_for_at_least(kBatchSize, std::chrono::milliseconds(500)));
  for (const Response &resp : collector.snapshot()) {
    EXPECT_FLOAT_EQ(resp.output[0], static_cast<float>(resp.id) * 10.0f);
  }
}
