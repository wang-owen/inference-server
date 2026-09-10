inference_server_add_module(
  NAME allocator
  SOURCES src/arena_allocator.cc src/slab_allocator.cc
  TEST_SOURCES tests/arena_allocator_test.cc tests/slab_allocator_test.cc
  BENCH_SOURCES benchmarks/allocator_bench.cc)
