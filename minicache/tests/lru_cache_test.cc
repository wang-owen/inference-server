#include "minicache/lru_cache.h"

#include <gtest/gtest.h>

TEST(LruCacheTest, ConstructsWithCapacity) {
  minicache::LruCache cache(16);
  EXPECT_EQ(cache.capacity(), 16u);
}

TEST(LruCacheTest, RejectsZeroCapacity) {
  EXPECT_THROW(minicache::LruCache(0), std::invalid_argument);
}

TEST(LruCacheTest, GetOnMissingKeyReturnsFalse) {
  minicache::LruCache cache(2);
  std::string out;
  EXPECT_FALSE(cache.get("missing", out));
}

TEST(LruCacheTest, PutThenGetRoundTrips) {
  minicache::LruCache cache(2);
  cache.put("a", "1");

  std::string out;
  ASSERT_TRUE(cache.get("a", out));
  EXPECT_EQ(out, "1");
  EXPECT_EQ(cache.size(), 1u);
}

TEST(LruCacheTest, PutOverwritesExistingKeyWithoutGrowing) {
  minicache::LruCache cache(2);
  cache.put("a", "1");
  cache.put("a", "2");

  std::string out;
  ASSERT_TRUE(cache.get("a", out));
  EXPECT_EQ(out, "2");
  EXPECT_EQ(cache.size(), 1u);
}

TEST(LruCacheTest, RemoveDeletesKey) {
  minicache::LruCache cache(2);
  cache.put("a", "1");

  EXPECT_TRUE(cache.remove("a"));
  EXPECT_EQ(cache.size(), 0u);

  std::string out;
  EXPECT_FALSE(cache.get("a", out));
}

TEST(LruCacheTest, RemoveOnMissingKeyReturnsFalse) {
  minicache::LruCache cache(2);
  EXPECT_FALSE(cache.remove("missing"));
}

TEST(LruCacheTest, PutOverCapacityEvictsLeastRecentlyUsed) {
  minicache::LruCache cache(2);
  cache.put("a", "1");
  cache.put("b", "2");
  cache.put("c", "3"); // cache was full with [a, b]; a is least-recently-used.

  std::string out;
  EXPECT_FALSE(cache.get("a", out));
  ASSERT_TRUE(cache.get("b", out));
  EXPECT_EQ(out, "2");
  ASSERT_TRUE(cache.get("c", out));
  EXPECT_EQ(out, "3");
  EXPECT_EQ(cache.size(), 2u);
}

TEST(LruCacheTest, GetPromotesEntrySoItSurvivesNextEviction) {
  minicache::LruCache cache(2);
  cache.put("a", "1");
  cache.put("b", "2");

  std::string out;
  ASSERT_TRUE(cache.get("a", out)); // a is now most-recently-used; b is LRU.

  cache.put("c", "3"); // should evict b, not a.

  EXPECT_FALSE(cache.get("b", out));
  ASSERT_TRUE(cache.get("a", out));
  EXPECT_EQ(out, "1");
  ASSERT_TRUE(cache.get("c", out));
  EXPECT_EQ(out, "3");
}
