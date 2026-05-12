// Example: resource hub metrics.
//
// Builds a hub, applies a small op stream, and prints the metrics summary.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    ops.append(OpKind::HandleInsert, 1, 1, 0, 0);
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    ops.append(OpKind::StatsSample, 1, 1, 4, 0);
    ops.append(OpKind::BatchStage, 1, 4, 16, 0);
    ops.append(OpKind::BatchCommit, 1, 0, 0, 0);
    hub.applyOps(ops);
    HubMetrics m = hub.collectMetrics();
    std::printf("arena.blocks=%u pool.slabs=%u handles.live=%u\n",
                m.arena.blocks, m.pool.slabs, m.handles.live);
    std::printf("epoch.current=%u pins.entries=%u buckets=%u\n",
                m.epoch.current, m.pins.entries, m.buckets);
    hub.destroy();
    return 0;
}
