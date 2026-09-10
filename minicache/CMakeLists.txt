inference_server_add_module(
  NAME minicache
  SOURCES src/lru_cache.cc src/command_dispatcher.cc
  DEPS netserver_lib
  MAIN main.cc
  TEST_SOURCES tests/lru_cache_test.cc tests/command_dispatcher_test.cc
  BENCH_SOURCES benchmarks/minicache_bench.cc)
