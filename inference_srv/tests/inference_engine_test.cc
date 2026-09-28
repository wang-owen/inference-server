#include "inference_srv/inference_engine.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using inference_srv::InferenceEngine;
using inference_srv::Request;
using inference_srv::Response;

TEST_CASE("ConstructsCleanly", "[inference_engine]") {
  InferenceEngine engine;
  SUCCEED();
}

TEST_CASE("ProducesOneResponsePerRequestInOrderWithMatchingIds",
          "[inference_engine]") {
  InferenceEngine engine;
  std::vector<Request> batch = {
      Request{.id = 1, .input = {1.0f, 2.0f}},
      Request{.id = 2, .input = {3.0f}},
      Request{.id = 3, .input = {}},
  };

  std::vector<Response> responses;
  engine.run_batch(batch, responses);

  REQUIRE(responses.size() == batch.size());
  for (std::size_t i = 0; i < batch.size(); ++i) {
    CHECK(responses[i].id == batch[i].id);
    REQUIRE(responses[i].output.size() == batch[i].input.size());
    for (std::size_t j = 0; j < batch[i].input.size(); ++j) {
      CHECK_THAT(responses[i].output[j],
                 Catch::Matchers::WithinULP(batch[i].input[j] * 2.0f, 4));
    }
  }
}

TEST_CASE("ReusesArenaAcrossSuccessiveBatches", "[inference_engine]") {
  InferenceEngine engine;

  for (int iteration = 0; iteration < 3; ++iteration) {
    std::vector<Request> batch = {
        Request{.id = static_cast<std::uint64_t>(iteration),
                .input = {1.0f, 2.0f, 3.0f}},
    };
    std::vector<Response> responses;
    engine.run_batch(batch, responses);

    REQUIRE(responses.size() == 1u);
    CHECK(responses[0].id == batch[0].id);
    CHECK_THAT(responses[0].output[0], Catch::Matchers::WithinULP(2.0f, 4));
    CHECK_THAT(responses[0].output[1], Catch::Matchers::WithinULP(4.0f, 4));
    CHECK_THAT(responses[0].output[2], Catch::Matchers::WithinULP(6.0f, 4));
  }
}

TEST_CASE("HandlesEmptyBatch", "[inference_engine]") {
  InferenceEngine engine;
  std::vector<Request> batch;
  std::vector<Response> responses;

  engine.run_batch(batch, responses);

  CHECK(responses.empty());
}
