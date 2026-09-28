#include "minicache/command_dispatcher.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ConstructsWithCache", "[command_dispatcher]") {
  minicache::LruCache cache(16);
  minicache::CommandDispatcher dispatcher(cache);
  SUCCEED();
}

TEST_CASE("SetReturnsOk", "[command_dispatcher]") {
  minicache::LruCache cache(16);
  minicache::CommandDispatcher dispatcher(cache);
  CHECK(dispatcher.dispatch("SET foo bar") == "OK");
}

TEST_CASE("GetAfterSetReturnsValue", "[command_dispatcher]") {
  minicache::LruCache cache(16);
  minicache::CommandDispatcher dispatcher(cache);
  dispatcher.dispatch("SET foo bar");
  CHECK(dispatcher.dispatch("GET foo") == "bar");
}

TEST_CASE("GetOnMissingKeyReturnsNil", "[command_dispatcher]") {
  minicache::LruCache cache(16);
  minicache::CommandDispatcher dispatcher(cache);
  CHECK(dispatcher.dispatch("GET missing") == "(nil)");
}

TEST_CASE("DelRemovesKey", "[command_dispatcher]") {
  minicache::LruCache cache(16);
  minicache::CommandDispatcher dispatcher(cache);
  dispatcher.dispatch("SET foo bar");
  CHECK(dispatcher.dispatch("DEL foo") == "OK");
  CHECK(dispatcher.dispatch("GET foo") == "(nil)");
}

TEST_CASE("DelOnMissingKeyReturnsFail", "[command_dispatcher]") {
  minicache::LruCache cache(16);
  minicache::CommandDispatcher dispatcher(cache);
  CHECK(dispatcher.dispatch("DEL missing") == "FAIL");
}

TEST_CASE("SetWithMissingArgumentsReturnsError", "[command_dispatcher]") {
  minicache::LruCache cache(16);
  minicache::CommandDispatcher dispatcher(cache);
  CHECK(dispatcher.dispatch("SET foo") ==
        "ERR wrong number of arguments for 'SET'");
}

TEST_CASE("GetWithMissingArgumentsReturnsError", "[command_dispatcher]") {
  minicache::LruCache cache(16);
  minicache::CommandDispatcher dispatcher(cache);
  CHECK(dispatcher.dispatch("GET") ==
        "ERR wrong number of arguments for 'GET'");
}

TEST_CASE("UnknownCommandReturnsError", "[command_dispatcher]") {
  minicache::LruCache cache(16);
  minicache::CommandDispatcher dispatcher(cache);
  CHECK(dispatcher.dispatch("FOO bar") == "ERR unknown command 'FOO'");
}
