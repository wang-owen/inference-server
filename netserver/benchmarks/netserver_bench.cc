#include "netserver/netserver.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("TcpListener lifecycle", "[!benchmark][netserver]") {
  BENCHMARK("TcpListener construct/destruct") {
    netserver::TcpListener listener("127.0.0.1", 0);
    return listener.listen_fd();
  };
}
