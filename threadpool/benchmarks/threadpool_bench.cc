#include "threadpool/thread_pool.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>

namespace {

double to_ms(std::chrono::steady_clock::duration d) {
  return std::chrono::duration<double, std::milli>(d).count();
}

void run_serial(int task_count) {
  std::atomic<int> counter{0};
  for (int i = 0; i < task_count; ++i)
    counter.fetch_add(1, std::memory_order_relaxed);
}

void run_pooled(int task_count, std::size_t worker_count) {
  std::atomic<int> counter{0};
  threadpool::ThreadPool pool(worker_count);
  for (int i = 0; i < task_count; ++i) {
    pool.submit(
        [&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
  }
}

} // namespace

int main() {
  constexpr int kTasks = 100'000;
  const unsigned hw_threads = std::max(2u, std::thread::hardware_concurrency());

  auto start = std::chrono::steady_clock::now();
  run_serial(kTasks);
  std::printf("1 thread:   %.3f ms for %d tasks\n",
              to_ms(std::chrono::steady_clock::now() - start), kTasks);

  start = std::chrono::steady_clock::now();
  run_pooled(kTasks, hw_threads);
  std::printf("%u threads: %.3f ms for %d tasks\n", hw_threads,
              to_ms(std::chrono::steady_clock::now() - start), kTasks);
  return 0;
}
