#include "minicache/command_dispatcher.h"

#include <gtest/gtest.h>

TEST(CommandDispatcherTest, ConstructsWithCache) {
    minicache::LruCache cache(16);
    minicache::CommandDispatcher dispatcher(cache);
    SUCCEED();
}

TEST(CommandDispatcherTest, SetReturnsOk) {
    minicache::LruCache cache(16);
    minicache::CommandDispatcher dispatcher(cache);
    EXPECT_EQ(dispatcher.dispatch("SET foo bar"), "OK");
}

TEST(CommandDispatcherTest, GetAfterSetReturnsValue) {
    minicache::LruCache cache(16);
    minicache::CommandDispatcher dispatcher(cache);
    dispatcher.dispatch("SET foo bar");
    EXPECT_EQ(dispatcher.dispatch("GET foo"), "bar");
}

TEST(CommandDispatcherTest, GetOnMissingKeyReturnsNil) {
    minicache::LruCache cache(16);
    minicache::CommandDispatcher dispatcher(cache);
    EXPECT_EQ(dispatcher.dispatch("GET missing"), "(nil)");
}

TEST(CommandDispatcherTest, DelRemovesKey) {
    minicache::LruCache cache(16);
    minicache::CommandDispatcher dispatcher(cache);
    dispatcher.dispatch("SET foo bar");
    EXPECT_EQ(dispatcher.dispatch("DEL foo"), "OK");
    EXPECT_EQ(dispatcher.dispatch("GET foo"), "(nil)");
}

TEST(CommandDispatcherTest, DelOnMissingKeyReturnsFail) {
    minicache::LruCache cache(16);
    minicache::CommandDispatcher dispatcher(cache);
    EXPECT_EQ(dispatcher.dispatch("DEL missing"), "FAIL");
}

TEST(CommandDispatcherTest, SetWithMissingArgumentsReturnsError) {
    minicache::LruCache cache(16);
    minicache::CommandDispatcher dispatcher(cache);
    EXPECT_EQ(dispatcher.dispatch("SET foo"), "ERR wrong number of arguments for 'SET'");
}

TEST(CommandDispatcherTest, GetWithMissingArgumentsReturnsError) {
    minicache::LruCache cache(16);
    minicache::CommandDispatcher dispatcher(cache);
    EXPECT_EQ(dispatcher.dispatch("GET"), "ERR wrong number of arguments for 'GET'");
}

TEST(CommandDispatcherTest, UnknownCommandReturnsError) {
    minicache::LruCache cache(16);
    minicache::CommandDispatcher dispatcher(cache);
    EXPECT_EQ(dispatcher.dispatch("FOO bar"), "ERR unknown command 'FOO'");
}
