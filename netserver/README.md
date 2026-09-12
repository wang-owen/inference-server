# netserver

Reactor-pattern TCP server: `EventLoop` blocks on a `Poller` (currently
`poll()`-backed; `epoll` can be dropped in later behind the same interface)
and dispatches ready `Channel`s. `Acceptor` accepts connections
non-blockingly and hands each new fd to a callback.

## Layout
    include/netserver/  public headers (#include "netserver/...")
    src/                 implementation
    main.cc             tiny demo executable
    tests/                GoogleTest suite
    benchmarks/           std::chrono micro-benchmarks (not run by ctest)

## Components
- `TcpListener` — raw listening socket (`socket`/`bind`/`listen`), no
  awareness of loops or threads.
- `Poller` / `PollPoller` — abstracts the OS notification mechanism behind
  `poll(timeout, &active_channels)`; hides `pollfd` bookkeeping from
  `Channel`/`EventLoop` so an `epoll`-backed `Poller` can later replace it.
- `Channel` — one fd's registered interest (`events_`) and the callback to
  run when the poller reports it ready (`revents_`). Does not own the fd.
- `EventLoop` — owns the `Poller` and the fd→`Channel*` map; `run()` is the
  reactor loop; a self-pipe (`wakeup()`/`handle_wakeup_read()`) lets
  `stop()` interrupt a blocked `poll()` call from another thread.
- `Acceptor` — owns a `TcpListener` and a `Channel` wrapping its fd;
  non-blockingly `accept()`s in a loop until `EAGAIN`, invoking
  `new_connection_callback_` for each new connection.
