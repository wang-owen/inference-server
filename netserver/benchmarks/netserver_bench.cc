#include "netserver/netserver.h"

#include <chrono>
#include <cstdio>

int main() {
  constexpr int kIterations = 100'000;
  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < kIterations; ++i) {
    netserver::TcpListener listener("127.0.0.1", 0);
    (void)listener;
  }
  auto elapsed = std::chrono::steady_clock::now() - start;
  std::printf("TcpListener construct/destruct: %.3f ms for %d iterations\n",
              std::chrono::duration<double, std::milli>(elapsed).count(),
              kIterations);
  return 0;
}
