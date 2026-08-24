#include "allocator/arena_allocator.h"

#include <cstdint>

#include <gtest/gtest.h>

TEST(ArenaAllocatorTest, ConstructsWithCapacity) {
  allocator::ArenaAllocator arena(256);
  EXPECT_EQ(arena.capacity(), 256u);
  EXPECT_EQ(arena.used(), 0u);
}

TEST(ArenaAllocatorTest, ResetDoesNotCrash) {
  allocator::ArenaAllocator arena(256);
  arena.reset();
  SUCCEED();
}

TEST(ArenaAllocatorTest, AllocateReturnsNonNull) {
  allocator::ArenaAllocator arena(256);
  EXPECT_EQ(arena.capacity(), 256u);
  EXPECT_NE(arena.allocate(1), nullptr);
}

TEST(ArenaAllocatorTest, ExhaustReturnsNull) {
  allocator::ArenaAllocator arena(256);
  EXPECT_EQ(arena.capacity(), 256u);
  EXPECT_NE(arena.allocate(256), nullptr);
  EXPECT_EQ(arena.allocate(1), nullptr);
}

TEST(ArenaAllocatorTest, ResetResetsSpace) {
  allocator::ArenaAllocator arena(256);
  EXPECT_EQ(arena.capacity(), 256u);
  EXPECT_NE(arena.allocate(256), nullptr);
  arena.reset();
  EXPECT_NE(arena.allocate(256), nullptr);
}

TEST(ArenaAllocatorTest, AllocateReturnsAlignedPointers) {
  allocator::ArenaAllocator arena(256);

  // Force misalignment first so the second allocate() has to round up.
  void *p1 = arena.allocate(1, 16);
  void *p2 = arena.allocate(3, 16);

  ASSERT_NE(p1, nullptr);
  ASSERT_NE(p2, nullptr);
  EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p1) % 16, 0u);
  EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p2) % 16, 0u);
}
