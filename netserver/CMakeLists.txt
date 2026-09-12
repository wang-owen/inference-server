inference_server_add_module(
  NAME netserver
  SOURCES src/tcp_server.cc src/tcp_listener.cc
  DEPS Threads::Threads
  MAIN main.cc
  TEST_SOURCES tests/tcp_server_test.cc tests/tcp_listener_test.cc
  BENCH_SOURCES benchmarks/netserver_bench.cc)
