#include "allocator/arena_allocator.h"
#include "allocator/slab_allocator.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

TEST_CASE("64-byte allocation", "[!benchmark][allocator]") {
  allocator::SlabAllocator slab(64, 1024);

  BENCHMARK("SlabAllocator alloc/free pair") {
    void *p = slab.allocate();
    slab.deallocate(p);
    return p;
  };

  // Catch2 picks the call count, so a fixed-size arena would run dry. Rewind
  // it when full; the reset cost is spread over 4096 allocations.
  allocator::ArenaAllocator arena(64 * 4096);
  BENCHMARK("ArenaAllocator allocate") {
    void *p = arena.allocate(64);
    if (p == nullptr) {
      arena.reset();
      p = arena.allocate(64);
    }
    return p;
  };

  BENCHMARK("malloc/free pair") {
    void *p = std::malloc(64);
    Catch::Benchmark::keep_memory(p);
    std::free(p);
  };
}
