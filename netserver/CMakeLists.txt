inference_server_add_module(
  NAME netserver
  SOURCES src/tcp_listener.cc src/channel.cc
          src/poller_poll.cc src/default_poller.cc src/event_loop.cc
          src/acceptor.cc
  DEPS Threads::Threads
  MAIN main.cc
  TEST_SOURCES tests/tcp_listener_test.cc tests/acceptor_test.cc
  BENCH_SOURCES benchmarks/netserver_bench.cc)
