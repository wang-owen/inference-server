#pragma once

#include <string>

#include "minicache/lru_cache.h"

namespace minicache {

// Parses a single text-protocol line and dispatches it to an LruCache,
// using a Redis-like wire protocol:
//   SET key value\r\n
//   GET key\r\n
//   DEL key\r\n
class CommandDispatcher {
public:
  explicit CommandDispatcher(LruCache &cache);

  // Parses and executes one line (without the trailing \r\n), returning
  // the text to write back to the client (e.g. "OK", "value", "(nil)").
  std::string dispatch(const std::string &line);

private:
  LruCache &cache_;
};

} // namespace minicache
