#include "minicache/lru_cache.h"

#include <stdexcept>

namespace minicache {

LruCache::LruCache(std::size_t capacity) : capacity_(capacity) {
  if (capacity_ == 0) {
    throw std::invalid_argument("LruCache: capacity must be greater than 0");
  }
}

bool LruCache::get(const std::string &key, std::string &out) {
  auto it = index_.find(key);
  if (it == index_.cend()) {
    return false;
  }

  out = it->second->second;
  order_.splice(order_.begin(), order_, it->second);
  return true;
}

void LruCache::put(const std::string &key, const std::string &value) {
  auto it = index_.find(key);
  if (it != index_.cend()) {
    order_.erase(it->second);
  }

  if (size() >= capacity_) {
    std::string to_erase = order_.back().first;
    order_.pop_back();
    index_.erase(to_erase);
  }

  order_.emplace_front(key, value);
  index_[key] = order_.begin();
}

bool LruCache::remove(const std::string &key) {
  auto it = index_.find(key);
  if (it == index_.cend()) {
    return false;
  }

  order_.erase(it->second);
  index_.erase(it);
  return true;
}

std::size_t LruCache::size() const { return order_.size(); }

} // namespace minicache
