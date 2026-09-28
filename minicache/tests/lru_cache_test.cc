#include "minicache/lru_cache.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ConstructsWithCapacity", "[lru_cache]") {
  minicache::LruCache cache(16);
  CHECK(cache.capacity() == 16u);
}

TEST_CASE("RejectsZeroCapacity", "[lru_cache]") {
  CHECK_THROWS_AS(minicache::LruCache(0), std::invalid_argument);
}

TEST_CASE("GetOnMissingKeyReturnsFalse", "[lru_cache]") {
  minicache::LruCache cache(2);
  std::string out;
  CHECK_FALSE(cache.get("missing", out));
}

TEST_CASE("PutThenGetRoundTrips", "[lru_cache]") {
  minicache::LruCache cache(2);
  cache.put("a", "1");

  std::string out;
  REQUIRE(cache.get("a", out));
  CHECK(out == "1");
  CHECK(cache.size() == 1u);
}

TEST_CASE("PutOverwritesExistingKeyWithoutGrowing", "[lru_cache]") {
  minicache::LruCache cache(2);
  cache.put("a", "1");
  cache.put("a", "2");

  std::string out;
  REQUIRE(cache.get("a", out));
  CHECK(out == "2");
  CHECK(cache.size() == 1u);
}

TEST_CASE("RemoveDeletesKey", "[lru_cache]") {
  minicache::LruCache cache(2);
  cache.put("a", "1");

  CHECK(cache.remove("a"));
  CHECK(cache.size() == 0u);

  std::string out;
  CHECK_FALSE(cache.get("a", out));
}

TEST_CASE("RemoveOnMissingKeyReturnsFalse", "[lru_cache]") {
  minicache::LruCache cache(2);
  CHECK_FALSE(cache.remove("missing"));
}

TEST_CASE("PutOverCapacityEvictsLeastRecentlyUsed", "[lru_cache]") {
  minicache::LruCache cache(2);
  cache.put("a", "1");
  cache.put("b", "2");
  cache.put("c", "3"); // cache was full with [a, b]; a is least-recently-used.

  std::string out;
  CHECK_FALSE(cache.get("a", out));
  REQUIRE(cache.get("b", out));
  CHECK(out == "2");
  REQUIRE(cache.get("c", out));
  CHECK(out == "3");
  CHECK(cache.size() == 2u);
}

TEST_CASE("GetPromotesEntrySoItSurvivesNextEviction", "[lru_cache]") {
  minicache::LruCache cache(2);
  cache.put("a", "1");
  cache.put("b", "2");

  std::string out;
  REQUIRE(cache.get("a", out)); // a is now most-recently-used; b is LRU.

  cache.put("c", "3"); // should evict b, not a.

  CHECK_FALSE(cache.get("b", out));
  REQUIRE(cache.get("a", out));
  CHECK(out == "1");
  REQUIRE(cache.get("c", out));
  CHECK(out == "3");
}
