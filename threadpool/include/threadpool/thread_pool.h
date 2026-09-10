#pragma once

#include <atomic>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>

namespace threadpool {

using Task = std::function<void()>;

class ThreadPool;

class WorkStealingDeque {
public:
  void push(Task task);

  bool try_pop(Task &task);

  bool try_steal(Task &task);

private:
  std::deque<Task> dq;
  mutable std::mutex mtx;
};

struct Worker {
  explicit Worker(ThreadPool *pool, std::size_t i) : pool{pool}, pool_idx{i} {}
  void run();
  bool steal() const;

  ThreadPool *pool;
  std::size_t pool_idx;
  WorkStealingDeque local_deque;
};

class ThreadPool {
public:
  // Throws std::invalid_argument if num_workers is 0.
  explicit ThreadPool(std::size_t num_workers);
  ~ThreadPool();
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  // Tasks must handle their own exceptions. An exception escaping a task is
  // treated as a bug and calls std::terminate.
  void submit(Task task);

  std::size_t num_workers() const { return workers_.size(); }

private:
  friend struct Worker;

  void stop_and_join() noexcept;

  std::vector<std::unique_ptr<Worker>> workers_;
  std::vector<std::jthread> threads_;
  std::atomic<std::size_t> round_robin_idx_ = 0;
  std::atomic<bool> shutdown_ = false;
  std::counting_semaphore<> wake_signal_{0};
};

} // namespace threadpool
