# inference-server

A high-throughput inference-serving stack in C++.
Each folder is its own Meson static library, and later components link against earlier ones instead of copy-pasting.

| Folder | Description | Depends on |
|---|---|---|
| [`allocator/`](allocator) | Arena and slab allocators | -- |
| [`threadpool/`](threadpool) | Work-stealing thread pool | -- |
| [`netserver/`](netserver) | Reactor-pattern TCP server | -- |
| [`minicache/`](minicache) | LRU cache server ("mini Redis") | netserver |
| [`inference_srv/`](inference_srv) | Batching inference server | allocator, threadpool, netserver |

## Layout

Every component follows the same shape:

    <component>/
      include/<component>/   public headers, #include "<component>/..."
      src/                    implementation
      main.cc                thin demo/server executable
      tests/                   Catch2 suite
      benchmarks/              Catch2 benchmarks (run via `meson test --benchmark`)
      meson.build
      README.md                architecture and component overview

## Build

Requires Meson and Ninja (`brew install meson`). Catch2 is fetched automatically
through `subprojects/catch2.wrap`.

```sh
meson setup builddir
meson compile -C builddir
meson test -C builddir              # run all unit tests
meson test -C builddir --benchmark  # run all benchmarks (add -v to see Catch2's tables)
./builddir/allocator/allocator_bench   # run one benchmark (pattern: builddir/<component>/<component>_bench)
```

Build options (both default `enabled`): `-Dtests=disabled`, `-Dbenchmarks=disabled`.
Sanitizers use Meson's built-in option: `-Db_sanitize=thread` or `-Db_sanitize=address`.
