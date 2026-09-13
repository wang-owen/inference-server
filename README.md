# inference-server

A high-throughput inference-serving stack in C++.
Each folder is its own CMake library, and later components link against earlier ones instead of copy-pasting.

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
      tests/                   GoogleTest suite
      benchmarks/              std::chrono micro-benchmarks (not run by ctest)
      CMakeLists.txt
      README.md                architecture and component overview

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build          # run all unit tests
./build/allocator/allocator_bench   # run a benchmark (pattern: build/<component>/<component>_bench)
```

Build options (both default `ON`): `-DINFERENCE_SERVER_BUILD_TESTS=OFF`,
`-DINFERENCE_SERVER_BUILD_BENCHMARKS=OFF`.
