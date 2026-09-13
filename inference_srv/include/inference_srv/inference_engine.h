#pragma once

#include <vector>

#include "allocator/arena_allocator.h"
#include "inference_srv/batching_queue.h"

namespace inference_srv {

// Stub "model": no real inference
class InferenceEngine {
public:
  InferenceEngine();

  void run_batch(const std::vector<Request> &batch,
                 std::vector<Response> &out_responses);

private:
  allocator::ArenaAllocator batch_arena_;
};

} // namespace inference_srv
