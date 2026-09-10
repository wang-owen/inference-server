#include "minicache/command_dispatcher.h"

#include <sstream>
#include <string>

namespace minicache {

CommandDispatcher::CommandDispatcher(LruCache &cache) : cache_(cache) {}

std::string CommandDispatcher::dispatch(const std::string &line) {
  std::istringstream iss{line};
  std::string command;
  iss >> command;

  if (command == "SET") {
    std::string key, value;
    if (!(iss >> key >> value)) {
      return "ERR wrong number of arguments for 'SET'";
    }
    cache_.put(key, value);
    return "OK";
  }

  if (command == "GET") {
    std::string key, value;
    if (!(iss >> key)) {
      return "ERR wrong number of arguments for 'GET'";
    }
    return cache_.get(key, value) ? value : "(nil)";
  }

  if (command == "DEL") {
    std::string key;
    if (!(iss >> key)) {
      return "ERR wrong number of arguments for 'DEL'";
    }
    return cache_.remove(key) ? "OK" : "FAIL";
  }

  return "ERR unknown command '" + command + "'";
}

} // namespace minicache
