// Example: server pool workflow.
//
// Allocates many small objects from a slab cache, tracks them via handles,
// releases them in transactional scopes, and prints a summary. This example
// shows how a typical game-server engine wires memforge.

#include <cstdio>
#include <vector>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();

    OpStream ops;
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    ops.append(OpKind::PoolAlloc, 1, 0, 0, 0);
    ops.append(OpKind::PoolAlloc, 2, 0, 0, 0);
    ops.append(OpKind::HandleInsert, 1, 0, 0, 0);
    ops.append(OpKind::HandleInsert, 2, 0, 0, 0);
    ops.append(OpKind::StatsSample, 1, 1, 32, 0);
    ops.append(OpKind::BatchStage, 1, 8, 16, 0);
    ops.append(OpKind::BatchCommit, 1, 0, 0, 0);
    ops.append(OpKind::SpanGuardAttach, 1, 2, 0, 0);
    ops.append(OpKind::SpanGuardVerify, 1, 0, 0, 0);

    hub.applyOps(ops);

    std::printf("server pool summary:\n");
    std::printf("  handles       = %zu\n", hub.handles().size());
    std::printf("  pool slabs    = %zu\n", hub.pool().totalSlabs());
    std::printf("  batch pending = %zu\n", hub.batches().pending());
    std::printf("  guard entries = %zu\n", hub.guards().size());
    hub.destroy();
    return 0;
}
