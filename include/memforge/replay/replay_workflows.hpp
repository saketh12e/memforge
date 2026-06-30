// MemForge - production replay workflows.
//
// The replay layer integrates ResourceHub and its modules in real
// production workflow APIs. Each replay class corresponds to one fuzzer
// target and drives a distinct subset of modules through a coherent use case.

#ifndef MEMFORGE_REPLAY_WORKFLOWS_HPP
#define MEMFORGE_REPLAY_WORKFLOWS_HPP

#include <memforge/resource_hub.hpp>
#include <memforge/op_stream.hpp>
#include <memforge/slab_cache.hpp>
#include <memforge/handle_table.hpp>
#include <memforge/epoch_queue.hpp>
#include <memforge/deferred_free_list.hpp>
#include <memforge/tx_scope.hpp>
#include <memforge/pool_migrator.hpp>
#include <memforge/pin_registry.hpp>
#include <memforge/stats_aggregator.hpp>
#include <memforge/checkpoint.hpp>
#include <memforge/span_guard.hpp>
#include <memforge/batch_allocator.hpp>
#include <memforge/shard_map.hpp>

namespace memforge {

// Each replay class accepts a decoded op stream and executes a workflow.
// The op stream drives the public surface; the workflow class invokes
// additional production APIs that the op stream cannot express.

class ArenaReplay {
public:
    Status runFinalize(ResourceHub& hub, const OpStream& ops);
};

class PoolReplay {
public:
    Status runLifecycle(ResourceHub& hub, const OpStream& ops);
    Status runWithMigrator(ResourceHub& hub, SlabCache& extra,
                            const OpStream& ops);
};

class HandleReplay {
public:
    Status runResolveCycle(ResourceHub& hub, const OpStream& ops);
    Status runResolveWithGuard(ResourceHub& hub, const OpStream& ops);
};

class EpochReplay {
public:
    Status runRetireFlush(ResourceHub& hub, const OpStream& ops);
};

class ScopeReplay {
public:
    Status runRollbackTeardown(ResourceHub& hub, const OpStream& ops);
};

class MigrationReplay {
public:
    Status runPlanGrowCommit(ResourceHub& hub, SlabCache& source,
                             SlabCache& dest, const OpStream& ops);
};

class StatsReplay {
public:
    Status runFoldExport(ResourceHub& hub, const OpStream& ops);
};

class CheckpointReplay {
public:
    Status runSnapshot(ResourceHub& hub, const OpStream& ops);
    Status runWithPins(ResourceHub& hub, const OpStream& ops);
};

class PinReplay {
public:
    Status runSnapshotRelease(ResourceHub& hub, const OpStream& ops);
};

class DeferredReplay {
public:
    Status runCompactFlush(ResourceHub& hub, const OpStream& ops);
};

class CompactReplay {
public:
    Status runPoolShardCompact(ResourceHub& hub, SlabCache& pool,
                                ShardMap& shards, const OpStream& ops);
};

class ShardReplay {
public:
    Status runRebalance(ResourceHub& hub, SlabCache& a, SlabCache& b,
                         ShardMap& shards, const OpStream& ops);
};

class GuardReplay {
public:
    Status runHandleGuardCheck(ResourceHub& hub, const OpStream& ops);
};

class BatchReplay {
public:
    Status runCommitCheckpoint(ResourceHub& hub, const OpStream& ops);
};

class TeardownReplay {
public:
    Status runPinnedEvictDestroy(ResourceHub& hub, const OpStream& ops);
};

}  // namespace memforge

#endif  // MEMFORGE_REPLAY_WORKFLOWS_HPP