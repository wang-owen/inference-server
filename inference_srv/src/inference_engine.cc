#include "inference_srv/inference_engine.h"

#include <chrono>
#include <stdexcept>
#include <thread>
#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <utility>

namespace inference_srv {

namespace {

constexpr std::size_t kBatchArenaBytes = 4 * 1024 * 1024;

constexpr auto kFixedOverheadPerBatch = std::chrono::milliseconds(5);

} // namespace

InferenceEngine::InferenceEngine() : batch_arena_{kBatchArenaBytes} {}

void InferenceEngine::run_batch(const std::vector<Request> &batch,
                                std::vector<Response> &out_responses) {
  out_responses.clear();
  out_responses.reserve(batch.size());

  for (const Request &req : batch) {
    std::size_t bytes = req.input.size() * sizeof(float);
    void *ptr = batch_arena_.allocate(bytes);
    if (ptr == nullptr) {
      throw std::runtime_error("allocate");
    }
    float *buf = static_cast<float *>(ptr);
    std::copy(req.input.begin(), req.input.end(), buf);

    Response resp;
    resp.id = req.id;
    resp.output.resize(req.input.size());

    // Scale by 2.0 as stand-in
    for (std::size_t i = 0; i < req.input.size(); ++i) {
      resp.output[i] = buf[i] * 2.0f;
    }

    out_responses.push_back(std::move(resp));
  }

  // Simulate model latency
  std::this_thread::sleep_for(kFixedOverheadPerBatch +
                              std::chrono::milliseconds(batch.size()));

  batch_arena_.reset();
}

} // namespace inference_srv
