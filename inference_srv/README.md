# inference_srv

Batching inference server: groups inference-style requests into batches
before processing, instead of one at a time. Same idea as vLLM/Triton's
dynamic batching. Uses [`allocator`](../allocator) directly for the
batch input buffer; `BatchingQueue`'s worker thread follows the same
mutex/condition_variable pattern as [`threadpool`](../threadpool)
without calling into it.

## WIP

- **No network front end.** There's no [`netserver`](../netserver)-backed
  listener yet -- `main.cc`, the tests, and the benchmark all call
  `BatchingQueue::submit()` in-process. Nothing currently accepts a
  request over a socket. `netserver_lib` is linked in
  `CMakeLists.txt` but unused in source.
- **`threadpool_lib` is also an unused link dependency** -- `BatchingQueue`
  hand-rolls its own single worker thread rather than calling `threadpool::ThreadPool`

## Why batching helps

A real inference call (and this stub's cost model) pays two kinds of
cost: a **fixed cost per call** (kernel launch, memory setup -- the same
whether the batch holds 1 request or 100) and a **variable cost per
item** (the actual compute, roughly linear in batch size). Processing
requests one at a time pays the fixed cost on every single request;
batching pays it once per batch and spreads it across every request in
that batch. The benchmark below shows the resulting speedup growing with
request volume, since more requests per batch means the fixed cost gets
divided further.

## Architecture: request flow

```
 client thread(s)                BatchingQueue                    worker thread
      |                               |                                |
      |-- submit(req, callback) ----->| push {req,cb} onto pending_,   |
      |                               | notify_one()                  |
      |                               |                                |
      |                               |                       wait_until(deadline,
      |                               |                         pending_.size() >= max_batch_size_
      |                               |                         || stop_)
      |                               |                                |
      |                               |                     drain up to max_batch_size_
      |                               |                     entries, release the lock
      |                               |                                |
      |                               |                     handler_(batch, out_responses) --.
      |                               |                                |                       |
      |                               |                                |                       v
      |                               |                                |            InferenceEngine::run_batch()
      |                               |                                |              - carve input buffer from
      |                               |                                |                batch_arena_ (allocator::ArenaAllocator)
      |                               |                                |              - stub compute (fixed +
      |                               |                                |                per-item sleep)
      |                               |                                |              - batch_arena_.reset()
      |                               |                                |<-- out_responses filled -------------'
      |                               |                                |
      |                               |                     match each Response.id back to
      |                               |                     its PendingRequest by id (not
      |                               |                     position -- handler order isn't
      |                               |                     guaranteed)
      |                               |                                |
      |<---------- on_response(matching Response) ------------------- |
```

Two independent conditions can trigger a flush, whichever comes first:
- **Size-triggered**: `pending_.size()` reaches `max_batch_size_`.
- **Time-triggered**: `max_wait_` elapses since `wait_until`'s deadline
  was set, even with a partial batch -- this bounds worst-case per-request
  latency when traffic is too sparse to fill a batch.

## Layout
    include/inference_srv/  public headers (#include "inference_srv/...")
    src/                     implementation
    main.cc                 tiny demo executable
    tests/                    GoogleTest suite
    benchmarks/               std::chrono micro-benchmarks (not run by ctest)

## Components
- `BatchingQueue` -- producer-consumer queue (mutex + condition_variable +
  a dedicated worker thread, in the spirit of `threadpool::ThreadPool`).
  `submit()` is the producer side, callable from any thread;
  `worker_loop()` is the single consumer that decides when to flush and
  drives the handler.
- `InferenceEngine` -- the stub "model." `run_batch()` processes a whole
  batch at once, using an `allocator::ArenaAllocator` sized for one
  batch's input buffer -- carve the batch's inputs out of one arena
  allocation instead of N heap allocations, then `reset()` the whole
  arena in O(1) once the batch is done.
- `BatchHandler` -- the `std::function` glue between the two: lets
  `BatchingQueue` stay ignorant of what "processing a batch" means, and
  lets tests inject a fake handler without touching `InferenceEngine`.

## Benchmark

`inference_bench.cc` compares naive one-at-a-time processing against
`BatchingQueue` (`max_batch_size=32`, `max_wait=10ms`) across request
counts, using `InferenceEngine`'s stub cost model (5ms fixed + 1ms/item):

    inference_srv throughput: naive (one-at-a-time) vs batched
        10 requests | naive:    73.69 ms | batched:    31.44 ms | speedup: 2.34x
        50 requests | naive:   372.36 ms | batched:    83.44 ms | speedup: 4.46x
       200 requests | naive:  1455.23 ms | batched:   272.06 ms | speedup: 5.35x
      1000 requests | naive:  7387.28 ms | batched:  1319.32 ms | speedup: 5.60x

Speedup grows with request volume rather than staying flat, which is the
signature of amortizing a fixed cost: as batches fill up, the fixed 5ms
gets divided across more requests, and the naive/batched ratio approaches
`max_batch_size` in the limit.
