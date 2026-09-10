# threadpool

Thread pool that pulls tasks off a shared queue and executes them across N
worker threads. Each worker loops on lock/wait/pop/execute until the pool
is destroyed, at which point it wakes and joins all workers.

## Layout
    include/threadpool/  public headers (#include "threadpool/...")
    src/                  implementation
    tests/                 GoogleTest suite
    benchmarks/            std::chrono micro-benchmarks (not run by ctest)
