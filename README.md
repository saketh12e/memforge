# MemForge

`memforge` is a C++17 memory-resource and object-lifecycle library for high-performance
servers, embedded runtimes, and in-memory caches. It provides:

- bump arenas with chained block metadata;
- slab pools with size classes, growth, and compaction;
- sharded class maps with cross-shard rebalancing;
- generational handle tables;
- epoch-based deferred reclamation;
- deferred free lists with compaction;
- transactional mutation scopes with rollback;
- cross-pool object migration;
- pin registries with shared payload ownership;
- per-class metric aggregation;
- binary resource checkpoints;
- a resource hub that owns and tears down the whole stack.

## Public API

Headers live under `include/memforge/`. Each module exposes a stable surface and a
sparse set of free functions for ergonomic use:

```cpp
#include <memforge/resource_hub.hpp>
#include <memforge/op_stream.hpp>

memforge::ResourceHub hub;
hub.init();

memforge::OpStream ops;
ops.append(memforge::ArenaAppendOp{/*...*/});
ops.append(memforge::PoolAllocOp{/*...*/});

hub.applyOps(ops);
hub.checkpoint("/tmp/memforge.ckpt");

hub.destroy();
```

A small CLI (`memforge_cli`) exposes `stats`, `inspect`, `checkpoint write`, `checkpoint read`,
and `migrate` for diagnostics. Examples live under `examples/` and tests under `tests/`.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Layout

```text
include/memforge/    public C++ headers
src/core/            errors, spans, checked math, op stream, span guard
src/arena/           arena chain, batch allocator
src/pool/            slab cache, shard map
src/handle/          generational handle table
src/epoch/           epoch queue, deferred free list
src/scope/           transactional scope
src/migrate/         pool migrator
src/pin/             pin registry
src/stats/           stats aggregator
src/checkpoint/      binary checkpoint writer/reader
src/hub/             resource hub
src/cli/             command-line tool
tests/               unit and integration tests
examples/            worked examples
fuzz/                libFuzzer harnesses and seed corpus
.clusterfuzzlite/    OSS-Fuzz / ClusterFuzzLite build glue
```

## License

Distributed under the terms described in `LICENSE`. See source headers for individual
attribution.
