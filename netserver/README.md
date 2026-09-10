# netserver

TCP server: `socket()`/`bind()`/`listen()`/`accept()`. Event-loop dispatch
(epoll) is deferred — see "Scope" below.

## Layout
    include/netserver/  public headers (#include "netserver/...")
    src/                 implementation
    main.cc             tiny demo executable
    tests/                GoogleTest suite
    benchmarks/           std::chrono micro-benchmarks (not run by ctest)

## Scope
This pass covers only the portable POSIX socket work (`TcpServer`), which
handles lifecycle (fd open/close), `start()`, and `accept_connection()`,
including against a raw client (`nc`). The epoll-based event loop
(`EventLoop`, non-blocking I/O, handling many concurrent connections on
one thread) is deferred to a later pass — it was scaffolded and then
removed to keep this pass focused. `TcpServer` itself is portable and
works as-is on macOS.
