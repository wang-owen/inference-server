# minicache

Networked key-value store (mini Redis) with a text protocol and LRU
eviction, built on top of [`netserver`](../netserver).

## Layout
    include/minicache/  public headers (#include "minicache/...")
    src/                   implementation
    main.cc               tiny demo executable
    tests/                  GoogleTest suite
    benchmarks/             std::chrono micro-benchmarks (not run by ctest)

## Design
`LruCache` implements O(1) get/put/remove with LRU eviction
(`std::list` + `std::unordered_map`, front = most-recently-used).
`CommandDispatcher` parses `SET`/`GET`/`DEL` lines and returns `OK`,
the value, `(nil)` on a GET miss, or an `ERR ...` string for malformed
or unrecognized commands. `main.cc` wires both into `netserver`'s
`EventLoop`/`Acceptor`: the reactor accepts connections non-blockingly,
and each accepted fd is handed to a detached thread running a blocking
`read()`/`send()` `handle_client()` loop — one thread per connection
rather than one connection at a time. Since `LruCache`/
`CommandDispatcher` aren't internally synchronized, `dispatcher.dispatch()`
calls are serialized across connection threads with a `std::mutex` in
`main.cc`. `SIGINT` triggers a graceful shutdown via `EventLoop::stop()`
(the same self-pipe wakeup mechanism `netserver` uses elsewhere).

Note: accepted sockets inherit `O_NONBLOCK` from the listening socket on
some platforms (observed on macOS/BSD; not on Linux), which would silently
turn `handle_client()`'s blocking `read()` into spurious `EAGAIN` errors —
`main.cc` explicitly restores blocking mode on each accepted fd before
handing it to its thread.
