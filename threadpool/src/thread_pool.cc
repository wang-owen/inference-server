#include "threadpool/thread_pool.h"

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace threadpool {

namespace {

// Tasks must not throw; see ThreadPool::submit()
void invoke_task(Task &task) noexcept { task(); }

} // namespace

void WorkStealingDeque::push(Task task) {
  std::lock_guard<std::mutex> lock(mtx);
  dq.push_back(std::move(task));
};

bool WorkStealingDeque::try_pop(Task &task) {
  std::lock_guard<std::mutex> lock(mtx);
  if (dq.empty())
    return false;
  task = std::move(dq.back());
  dq.pop_back();
  return true;
}

bool WorkStealingDeque::try_steal(Task &task) {
  std::lock_guard<std::mutex> lock(mtx);
  if (dq.empty())
    return false;
  task = std::move(dq.front());
  dq.pop_front();
  return true;
}

void Worker::run() {
  while (true) {
    Task task;
    if (local_deque.try_pop(task)) {
      invoke_task(task);
    } else if (!steal()) {
      if (pool->shutdown_.load(std::memory_order_relaxed))
        break;
      pool->wake_signal_.acquire();
    }
  }
}

bool Worker::steal() const {
  Task task;
  const std::size_t num_workers = pool->num_workers();
  for (std::size_t i = (pool_idx + 1) % num_workers;
       i != pool_idx % num_workers; i = (i + 1) % num_workers) {
    if (pool->workers_[i]->local_deque.try_steal(task)) {
      invoke_task(task);
      return true;
    }
  }
  return false;
}

ThreadPool::ThreadPool(std::size_t num_workers) {
  if (num_workers == 0)
    throw std::invalid_argument("ThreadPool requires at least one worker");

  workers_.reserve(num_workers);
  threads_.reserve(num_workers);
  for (std::size_t i = 0; i < num_workers; ++i) {
    workers_.push_back(std::make_unique<Worker>(this, i));
  }

  try {
    for (std::size_t i = 0; i < num_workers; ++i) {
      threads_.emplace_back([this, i] { workers_[i]->run(); });
    }
  } catch (...) {
    stop_and_join();
    throw;
  }
}

ThreadPool::~ThreadPool() { stop_and_join(); }

void ThreadPool::stop_and_join() noexcept {
  shutdown_.store(true, std::memory_order_release);
  wake_signal_.release(static_cast<std::ptrdiff_t>(threads_.size()));
  for (auto &t : threads_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void ThreadPool::submit(Task task) {
  if (shutdown_.load(std::memory_order_acquire)) {
    return;
  }

  Worker *worker =
      workers_[round_robin_idx_.fetch_add(1) % num_workers()].get();
  worker->local_deque.push(std::move(task));
  wake_signal_.release();
}

} // namespace threadpool
