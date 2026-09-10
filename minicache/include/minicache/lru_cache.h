#pragma once

#include <cstddef>
#include <list>
#include <string>
#include <unordered_map>

namespace minicache {

class LruCache {
public:
  // Throws std::invalid_argument if `capacity` is 0.
  explicit LruCache(std::size_t capacity);

  LruCache(const LruCache &) = delete;
  LruCache &operator=(const LruCache &) = delete;

  // Returns false and leaves `out` untouched if `key` isn't present. A
  // successful get marks `key` as most-recently-used.
  bool get(const std::string &key, std::string &out);

  // Inserts or overwrites `key`. Evicts the least-recently-used entry
  // first if the cache is already at capacity.
  void put(const std::string &key, const std::string &value);

  bool remove(const std::string &key);

  std::size_t size() const;
  std::size_t capacity() const { return capacity_; }

private:
  using Entry = std::pair<std::string, std::string>;

  // Front = most-recently-used, back = least-recently-used.
  std::list<Entry> order_;
  std::unordered_map<std::string, decltype(order_)::iterator> index_;
  std::size_t capacity_;
};

} // namespace minicache
