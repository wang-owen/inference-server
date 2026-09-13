inference_server_add_module(
  NAME inference_srv
  SOURCES src/batching_queue.cc src/inference_engine.cc
  DEPS allocator_lib threadpool_lib netserver_lib
  MAIN main.cc
  TEST_SOURCES tests/batching_queue_test.cc tests/inference_engine_test.cc
  BENCH_SOURCES benchmarks/inference_bench.cc)
