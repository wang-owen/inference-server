inference_server_add_module(
  NAME threadpool
  SOURCES src/thread_pool.cc
  DEPS Threads::Threads
  TEST_SOURCES tests/thread_pool_test.cc
  BENCH_SOURCES benchmarks/threadpool_bench.cc)
