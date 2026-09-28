#include "minicache/minicache.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string>
#include <vector>

TEST_CASE("LruCache operations", "[!benchmark][minicache]") {
  // Twice as many keys as the capacity, cycled in order, so steady state
  // evicts on every put.
  constexpr std::size_t kCapacity = 1024;
  std::vector<std::string> keys;
  keys.reserve(2 * kCapacity);
  for (std::size_t i = 0; i < 2 * kCapacity; ++i)
    keys.push_back("key" + std::to_string(i));

  minicache::LruCache cache(kCapacity);
  std::string out;
  std::size_t next = 0;

  BENCHMARK("put+get") {
    const std::string &key = keys[next++ % keys.size()];
    cache.put(key, "value");
    return cache.get(key, out);
  };
}
