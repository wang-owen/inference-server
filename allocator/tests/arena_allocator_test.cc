#include "allocator/arena_allocator.h"

#include <cstdint>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ConstructsWithCapacity", "[arena_allocator]") {
  allocator::ArenaAllocator arena(256);
  CHECK(arena.capacity() == 256u);
  CHECK(arena.used() == 0u);
}

TEST_CASE("ResetDoesNotCrash", "[arena_allocator]") {
  allocator::ArenaAllocator arena(256);
  arena.reset();
  SUCCEED();
}

TEST_CASE("AllocateReturnsNonNull", "[arena_allocator]") {
  allocator::ArenaAllocator arena(256);
  CHECK(arena.capacity() == 256u);
  CHECK(arena.allocate(1) != nullptr);
}

TEST_CASE("ExhaustReturnsNull", "[arena_allocator]") {
  allocator::ArenaAllocator arena(256);
  CHECK(arena.capacity() == 256u);
  CHECK(arena.allocate(256) != nullptr);
  CHECK(arena.allocate(1) == nullptr);
}

TEST_CASE("ResetResetsSpace", "[arena_allocator]") {
  allocator::ArenaAllocator arena(256);
  CHECK(arena.capacity() == 256u);
  CHECK(arena.allocate(256) != nullptr);
  arena.reset();
  CHECK(arena.allocate(256) != nullptr);
}

TEST_CASE("AllocateReturnsAlignedPointers", "[arena_allocator]") {
  allocator::ArenaAllocator arena(256);

  // Force misalignment first so the second allocate() has to round up.
  void *p1 = arena.allocate(1, 16);
  void *p2 = arena.allocate(3, 16);

  REQUIRE(p1 != nullptr);
  REQUIRE(p2 != nullptr);
  CHECK(reinterpret_cast<std::uintptr_t>(p1) % 16 == 0u);
  CHECK(reinterpret_cast<std::uintptr_t>(p2) % 16 == 0u);
}
