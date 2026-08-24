# allocator

Arena/slab allocator library, benchmarked against `malloc`.

## Benchmark

1,000,000 alloc/free pairs of 64-byte chunks (`-O3`, AppleClang 21):

| Allocator        | Time     | vs malloc |
|------------------|----------|-----------|
| `ArenaAllocator` | 5.6 ms   | ~2.7x faster |
| `SlabAllocator`  | 10.5 ms  | ~1.4x faster |
| `malloc`/`free`  | 14.8 ms  | baseline |

Numbers are platform-dependent — re-run `allocator_bench` locally rather
than trusting these exactly.

## Tradeoffs

- **Arena**: fastest — `allocate()` is a pointer bump, no bookkeeping. No
  per-object `free()`, only `reset()`, which invalidates everything at
  once. Fits allocations that share one lifetime (e.g. one request's
  scratch memory).
- **Slab**: close to `malloc`'s speed, since both do per-call bookkeeping,
  just simpler (free-list push/pop vs. general heap logic). Its win over
  `malloc` is no fragmentation — every chunk is the same size, so freed
  chunks always fit the next request. Fits repeated same-size alloc/free
  (e.g. fixed-size cache entries, KV-cache pages).
- **`malloc`**: most flexible — arbitrary sizes, individual lifetimes, no
  upfront capacity — which is exactly what costs it speed here.

