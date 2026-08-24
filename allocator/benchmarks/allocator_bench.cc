#include "allocator/arena_allocator.h"
#include "allocator/slab_allocator.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>

namespace {

void do_not_optimize(void *p) { asm volatile("" : : "r"(p) : "memory"); }

double to_ms(std::chrono::steady_clock::duration d) {
  return std::chrono::duration<double, std::milli>(d).count();
}

} // namespace

int main() {
  constexpr int kIterations = 1'000'000;

  auto start = std::chrono::steady_clock::now();
  allocator::SlabAllocator slab(64, kIterations);
  for (int i = 0; i < kIterations; ++i) {
    void *p = slab.allocate();
    do_not_optimize(p);
    slab.deallocate(p);
  }
  auto slab_elapsed = std::chrono::steady_clock::now() - start;

  start = std::chrono::steady_clock::now();
  allocator::ArenaAllocator arena(64 * kIterations);
  for (int i = 0; i < kIterations; ++i) {
    void *p = arena.allocate(64);
    do_not_optimize(p);
  }
  auto arena_elapsed = std::chrono::steady_clock::now() - start;

  start = std::chrono::steady_clock::now();
  for (int i = 0; i < kIterations; ++i) {
    void *p = std::malloc(64);
    do_not_optimize(p);
    std::free(p);
  }
  auto malloc_elapsed = std::chrono::steady_clock::now() - start;

  std::printf("SlabAllocator:  %.3f ms for %d alloc/free pairs\n",
              to_ms(slab_elapsed), kIterations);
  std::printf("ArenaAllocator: %.3f ms for %d allocate calls\n",
              to_ms(arena_elapsed), kIterations);
  std::printf("malloc/free:    %.3f ms for %d alloc/free pairs\n",
              to_ms(malloc_elapsed), kIterations);
  return 0;
}
