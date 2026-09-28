#include "allocator/slab_allocator.h"

#include <unordered_set>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ConstructsWithChunkSize", "[slab_allocator]") {
  allocator::SlabAllocator slab(32, 8);
  CHECK(slab.chunk_size() >= 32u);
}

TEST_CASE("AllocateReturnsDistinctNonNullPointers", "[slab_allocator]") {
  allocator::SlabAllocator slab(32, 8);
  CHECK(slab.free_count() == 8u);

  std::unordered_set<void *> seen;
  for (int i = 0; i < 8; ++i) {
    void *p = slab.allocate();
    REQUIRE(p != nullptr);
    INFO("duplicate chunk returned");
    CHECK(seen.insert(p).second);
  }
  CHECK(slab.free_count() == 0u);
}

TEST_CASE("ExhaustReturnsNull", "[slab_allocator]") {
  allocator::SlabAllocator slab(32, 8);
  for (int i = 0; i < 8; ++i) {
    CHECK(slab.allocate() != nullptr);
  }
  CHECK(slab.allocate() == nullptr);
}

TEST_CASE("DeallocateMakesChunkAllocatableAgain", "[slab_allocator]") {
  allocator::SlabAllocator slab(32, 8);
  void *first = slab.allocate();
  REQUIRE(first != nullptr);

  slab.deallocate(first);
  CHECK(slab.free_count() == 8u);

  void *reused = slab.allocate();
  CHECK(reused == first);
}
