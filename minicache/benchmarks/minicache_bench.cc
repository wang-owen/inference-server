#include "minicache/minicache.h"

#include <chrono>
#include <cstdio>
#include <string>

int main() {
  constexpr int kIterations = 100'000;
  minicache::LruCache cache(1024);
  std::string out;

  auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < kIterations; ++i) {
    cache.put("key" + std::to_string(i % 2048), "value");
    cache.get("key" + std::to_string(i % 2048), out);
  }
  auto elapsed = std::chrono::steady_clock::now() - start;
  std::printf("LruCache put+get: %.3f ms for %d iterations\n",
              std::chrono::duration<double, std::milli>(elapsed).count(),
              kIterations);
  return 0;
}
