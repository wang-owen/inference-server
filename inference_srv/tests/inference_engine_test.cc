#include "inference_srv/inference_engine.h"

#include <gtest/gtest.h>

using inference_srv::InferenceEngine;
using inference_srv::Request;
using inference_srv::Response;

TEST(InferenceEngineTest, ConstructsCleanly) {
  InferenceEngine engine;
  SUCCEED();
}

TEST(InferenceEngineTest, ProducesOneResponsePerRequestInOrderWithMatchingIds) {
  InferenceEngine engine;
  std::vector<Request> batch = {
      Request{.id = 1, .input = {1.0f, 2.0f}},
      Request{.id = 2, .input = {3.0f}},
      Request{.id = 3, .input = {}},
  };

  std::vector<Response> responses;
  engine.run_batch(batch, responses);

  ASSERT_EQ(responses.size(), batch.size());
  for (std::size_t i = 0; i < batch.size(); ++i) {
    EXPECT_EQ(responses[i].id, batch[i].id);
    ASSERT_EQ(responses[i].output.size(), batch[i].input.size());
    for (std::size_t j = 0; j < batch[i].input.size(); ++j) {
      EXPECT_FLOAT_EQ(responses[i].output[j], batch[i].input[j] * 2.0f);
    }
  }
}

TEST(InferenceEngineTest, ReusesArenaAcrossSuccessiveBatches) {
  InferenceEngine engine;

  for (int iteration = 0; iteration < 3; ++iteration) {
    std::vector<Request> batch = {
        Request{.id = static_cast<std::uint64_t>(iteration),
                .input = {1.0f, 2.0f, 3.0f}},
    };
    std::vector<Response> responses;
    engine.run_batch(batch, responses);

    ASSERT_EQ(responses.size(), 1u);
    EXPECT_EQ(responses[0].id, batch[0].id);
    EXPECT_FLOAT_EQ(responses[0].output[0], 2.0f);
    EXPECT_FLOAT_EQ(responses[0].output[1], 4.0f);
    EXPECT_FLOAT_EQ(responses[0].output[2], 6.0f);
  }
}

TEST(InferenceEngineTest, HandlesEmptyBatch) {
  InferenceEngine engine;
  std::vector<Request> batch;
  std::vector<Response> responses;

  engine.run_batch(batch, responses);

  EXPECT_TRUE(responses.empty());
}
