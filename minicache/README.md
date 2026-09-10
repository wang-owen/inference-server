# minicache

Networked key-value store (mini Redis) with a text protocol and LRU
eviction, built on top of [`netserver`](../netserver).

## Layout
    include/minicache/  public headers (#include "minicache/...")
    src/                   implementation
    main.cc               tiny demo executable
    tests/                  GoogleTest suite
    benchmarks/             std::chrono micro-benchmarks (not run by ctest)

## Status
Done. `LruCache` implements O(1) get/put/remove with LRU eviction
(`std::list` + `std::unordered_map`, front = most-recently-used).
`CommandDispatcher` parses `SET`/`GET`/`DEL` lines and returns `OK`,
the value, `(nil)` on a GET miss, or an `ERR ...` string for malformed
or unrecognized commands. `main.cc` wires both into `netserver`'s
`TcpServer`: it accepts one client at a time, accumulates bytes across
`read()` calls and splits on `\n` to handle partial or pipelined
commands, and closes the connection (without killing the server) on
disconnect or I/O error. `SIGINT` triggers a graceful shutdown via
`TcpServer::stop()`.

Verified end-to-end over a raw socket via `netcat`: pipelined
`SET`/`GET`/`DEL`/unknown-command sequences, eviction at capacity, and
sequential connections all behave correctly.
