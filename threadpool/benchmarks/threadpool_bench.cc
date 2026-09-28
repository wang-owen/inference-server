#include "threadpool/thread_pool.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <string>
#include <thread>

namespace {

constexpr int kTasks = 100'000;

int run_serial(int task_count) {
  std::atomic<int> counter{0};
  for (int i = 0; i < task_count; ++i)
    counter.fetch_add(1, std::memory_order_relaxed);
  return counter.load();
}

// Includes pool startup and teardown: the destructor joins every worker, which
// is the only way to wait for all submitted tasks to finish.
int run_pooled(int task_count, std::size_t worker_count) {
  std::atomic<int> counter{0};
  {
    threadpool::ThreadPool pool(worker_count);
    for (int i = 0; i < task_count; ++i) {
      pool.submit(
          [&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }
  }
  return counter.load();
}

} // namespace

TEST_CASE("100k trivial tasks", "[!benchmark][threadpool]") {
  const unsigned hw_threads = std::max(2u, std::thread::hardware_concurrency());

  BENCHMARK("1 thread (serial)") { return run_serial(kTasks); };

  BENCHMARK(std::to_string(hw_threads) + " worker ThreadPool") {
    return run_pooled(kTasks, hw_threads);
  };
}
