#include "threadpool/thread_pool.h"

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace {

bool wait_until(const std::function<bool()> &pred,
                std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (pred())
      return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return pred();
}

} // namespace

TEST_CASE("PopIsLifoAndStealIsFifo", "[work_stealing_deque]") {
  threadpool::WorkStealingDeque dq;
  std::vector<int> order;
  dq.push([&order] { order.push_back(1); });
  dq.push([&order] { order.push_back(2); });
  dq.push([&order] { order.push_back(3); });
  threadpool::Task task;
  REQUIRE(dq.try_pop(task));
  task(); // owner-thread pop takes from the back (LIFO): runs task 3
  REQUIRE(dq.try_steal(task));
  task(); // thief takes from the front (FIFO): runs task 1
  REQUIRE(dq.try_pop(task));
  task(); // only task 2 is left
  CHECK(order == std::vector<int>{3, 1, 2});
}

TEST_CASE("ConstructsRequestedWorkerCount", "[thread_pool]") {
  threadpool::ThreadPool pool(3);
  CHECK(pool.num_workers() == 3u);
}

TEST_CASE("RunsAllSubmittedTasks", "[thread_pool]") {
  std::atomic<int> counter{0};
  constexpr int kTasks = 1000;
  {
    threadpool::ThreadPool pool(4);
    for (int i = 0; i < kTasks; ++i) {
      pool.submit(
          [&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }
  } // destructor joins every worker, so all tasks have run by here
  CHECK(counter.load() == kTasks);
}

TEST_CASE("ThrowsOnZero", "[thread_pool]") {
  CHECK_THROWS(threadpool::ThreadPool(0));
}

TEST_CASE("IdleWorkerStealsFromBusyWorkersQueue", "[thread_pool]") {
  threadpool::ThreadPool pool(2);
  std::atomic<bool> release_blocked_task{false};
  std::atomic<int> completed{0};
  constexpr int kExtraTasks = 20;

  // Round-robin submission puts this first task on worker 0 and occupies its
  // thread until released, so worker 0 can never drain its own queue.
  pool.submit([&release_blocked_task] {
    while (!release_blocked_task.load(std::memory_order_acquire)) {
      std::this_thread::yield();
    }
  });

  // With 2 workers these alternate onto worker 1's queue and worker 0's
  // queue. If worker 1 never stole from worker 0, only half would finish.
  for (int i = 0; i < kExtraTasks; ++i) {
    pool.submit(
        [&completed] { completed.fetch_add(1, std::memory_order_relaxed); });
  }

  const bool all_completed = wait_until(
      [&] { return completed.load() == kExtraTasks; }, std::chrono::seconds(2));
  INFO("expected worker 1 to steal worker 0's backlog while worker 0 was "
       "blocked, but only "
       << completed.load() << "/" << kExtraTasks << " tasks completed");
  CHECK(all_completed);

  release_blocked_task.store(true, std::memory_order_release);
}

TEST_CASE("SubmitDuringShutdownIsDropped", "[thread_pool]") {
  std::atomic<bool> extra_task_ran{false};
  {
    threadpool::ThreadPool pool(1);
    threadpool::ThreadPool *pool_ptr = &pool;
    pool.submit([pool_ptr, &extra_task_ran] {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      pool_ptr->submit([&extra_task_ran] { extra_task_ran.store(true); });
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  CHECK_FALSE(extra_task_ran.load());
}

TEST_CASE("AcceptsConcurrentSubmissionsFromMultipleThreads", "[thread_pool]") {
  constexpr int kProducers = 8;
  constexpr int kTasksPerProducer = 500;
  std::atomic<int> counter{0};
  {
    threadpool::ThreadPool pool(4);
    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p) {
      producers.emplace_back([&pool, &counter] {
        for (int i = 0; i < kTasksPerProducer; ++i) {
          pool.submit(
              [&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
        }
      });
    }
    for (auto &t : producers) {
      t.join();
    }
  } // destructor joins every worker, so all tasks have run by here
  CHECK(counter.load() == kProducers * kTasksPerProducer);
}

TEST_CASE("TasksCarryingHeapAllocatedStateRunCorrectly", "[thread_pool]") {
  struct Payload {
    std::vector<int> data;
    int checksum() const {
      int sum = 0;
      for (int v : data) {
        sum += v;
      }
      return sum;
    }
  };

  constexpr int kTasks = 200;
  std::atomic<int> mismatches{0};
  {
    threadpool::ThreadPool pool(4);
    for (int i = 0; i < kTasks; ++i) {
      auto payload = std::make_shared<Payload>();
      payload->data = std::vector<int>(50, i);
      const int expected = payload->checksum();
      pool.submit([payload, expected, &mismatches] {
        if (payload->checksum() != expected) {
          mismatches.fetch_add(1, std::memory_order_relaxed);
        }
      });
    }
  }
  CHECK(mismatches.load() == 0);
}
