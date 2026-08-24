#include "allocator/slab_allocator.h"

#include <unordered_set>

#include <gtest/gtest.h>

TEST(SlabAllocatorTest, ConstructsWithChunkSize) {
  allocator::SlabAllocator slab(32, 8);
  EXPECT_GE(slab.chunk_size(), 32u);
}

TEST(SlabAllocatorTest, AllocateReturnsDistinctNonNullPointers) {
  allocator::SlabAllocator slab(32, 8);
  EXPECT_EQ(slab.free_count(), 8u);

  std::unordered_set<void *> seen;
  for (int i = 0; i < 8; ++i) {
    void *p = slab.allocate();
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(seen.insert(p).second) << "duplicate chunk returned";
  }
  EXPECT_EQ(slab.free_count(), 0u);
}

TEST(SlabAllocatorTest, ExhaustReturnsNull) {
  allocator::SlabAllocator slab(32, 8);
  for (int i = 0; i < 8; ++i) {
    EXPECT_NE(slab.allocate(), nullptr);
  }
  EXPECT_EQ(slab.allocate(), nullptr);
}

TEST(SlabAllocatorTest, DeallocateMakesChunkAllocatableAgain) {
  allocator::SlabAllocator slab(32, 8);
  void *first = slab.allocate();
  ASSERT_NE(first, nullptr);

  slab.deallocate(first);
  EXPECT_EQ(slab.free_count(), 8u);

  void *reused = slab.allocate();
  EXPECT_EQ(reused, first);
}
