// MemForge - replay workflow implementations.
//
// Each replay class orchestrates a real production scenario: it accepts a
// decoded op stream, applies it to the ResourceHub, and then invokes
// additional production APIs that exercise cross-module interactions.

#include <memforge/replay/replay_workflows.hpp>
#include <memforge/replay.hpp>

#include <cstdint>
#include <cstring>

namespace memforge {

namespace {

inline std::uint32_t as_u32(std::size_t v) noexcept {
    return static_cast<std::uint32_t>(v);
}

}  // namespace

Status ArenaReplay::runFinalize(ResourceHub& hub, const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    if (hub.arena().blockCount() > 0) {
        std::uint32_t blocks = as_u32(hub.arena().blockCount());
        hub.writer().begin(CheckpointBeginOp{1, blocks, 32});
        hub.writer().writeHandleTable(CheckpointWriteHandleTableOp{
            1, blocks, 32});
        hub.writer().writeArenaSection(hub.arena());
    }
    return Ok();
}

Status PoolReplay::runLifecycle(ResourceHub& hub, const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    SlabSlot slot{};
    if (hub.pool().alloc(0, slot) == Ok() && slot.base != nullptr) {
        hub.pins().pinSlotView(0, slot.base, 64);
        hub.pool().compact(0);
        hub.pins().flushSlotViews();
    }
    return Ok();
}

Status PoolReplay::runWithMigrator(ResourceHub& hub, SlabCache& extra,
                                   const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    hub.migrator().apply(extra, extra, ops);
    return Ok();
}

Status HandleReplay::runResolveCycle(ResourceHub& hub, const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    OpStream second;
    for (std::size_t i = 0; i < ops.size(); ++i) {
        if (ops[i].kind == OpKind::HandleResolve) {
            second.append(OpKind::HandleResolve, ops[i].a, ops[i].b,
                          ops[i].c, ops[i].d);
        }
    }
    applyToHub(hub, second);
    return Ok();
}

Status HandleReplay::runResolveWithGuard(ResourceHub& hub,
                                          const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    for (std::size_t i = 0; i < hub.handles().size(); ++i) {
        const HandleEntry& he = hub.handles().entry(i);
        if (he.slot_id == 0) {
            continue;
        }
        std::uint32_t packed =
            (he.generation << 16) | (he.slot_id & 0xFFFFu);
        std::uint32_t guard_class = 0;
        std::uint32_t transform = 0;
        if (hub.handles().resolveGuardClass(packed, guard_class,
                                              transform) != Ok()) {
            continue;
        }
        (void)hub.guards().patternForHandleClass(guard_class, transform);
    }
    return Ok();
}

Status EpochReplay::runRetireFlush(ResourceHub& hub, const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    std::vector<std::uint8_t*> transferred;
    std::vector<std::uint32_t> sizes;
    hub.epoch().drainToTransfer(1, transferred, sizes);
    for (std::size_t i = 0; i < transferred.size(); ++i) {
        delete[] transferred[i];
    }
    std::vector<std::uint8_t> summary;
    hub.epoch().cursorSummary(summary);
    return Ok();
}

Status ScopeReplay::runRollbackTeardown(ResourceHub& hub,
                                          const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    for (std::uint32_t id = 1; id <= 16; ++id) {
        if (hub.scope().scopeCount() > 0) {
            hub.scope().rollback(id);
        }
    }
    return Ok();
}

Status MigrationReplay::runPlanGrowCommit(ResourceHub& hub,
                                          SlabCache& source,
                                          SlabCache& dest,
                                          const OpStream& ops) {
    (void)hub;
    OpStream pool_only;
    OpStream plan_only;
    OpStream commit_only;
    OpStream rebalance_only;
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::MigratePlan:
            plan_only.append(op.kind, op.a, op.b, op.c, op.d);
            break;
        case OpKind::MigrateCommit:
            commit_only.append(op.kind, op.a, op.b, 0, 0);
            break;
        case OpKind::PoolCompact:
            rebalance_only.append(op.kind, op.a, op.b, op.c, op.d);
            break;
        case OpKind::ShardRebalance:
            rebalance_only.append(op.kind, op.a, op.b, op.c, op.d);
            break;
        default:
            pool_only.append(op.kind, op.a, op.b, op.c, op.d);
            break;
        }
    }
    source.apply(pool_only);
    dest.apply(pool_only);
    ShardMap local_shards;
    local_shards.init();
    local_shards.attachShard(0, &source);
    local_shards.attachShard(1, &dest);
    local_shards.registerClassMembership(0, 0);
    local_shards.registerClassMembership(1, 0);
    hub.migrator().apply(source, dest, plan_only);
    if (local_shards.shardCount() >= 2) {
        local_shards.rebalance(0, 1);
    }
    hub.migrator().apply(source, dest, commit_only);
    return Ok();
}

Status StatsReplay::runFoldExport(ResourceHub& hub, const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    SlabCache s1, s2;
    s1.init(); s2.init();
    hub.shards().attachShard(0, &s1);
    hub.shards().attachShard(1, &s2);
    hub.shards().registerClassMembership(0, 0);
    hub.shards().registerClassMembership(1, 0);
    for (std::uint32_t b = 0; b < kStatsBuckets; ++b) {
        hub.stats().foldMetrics(StatsFoldOp{b, 0});
        std::uint32_t plan_size = static_cast<std::uint32_t>(
            hub.stats().foldPlanSize());
        if (plan_size == 0) {
            continue;
        }
        hub.writer().beginStatsSection(plan_size, sizeof(StatsGroup));
        for (std::uint32_t group_id : hub.stats().foldPlanGroupIds()) {
            auto aliases = hub.shards().classAliasesForBucket(group_id);
            for (std::uint32_t shard : aliases) {
                std::uint8_t entry[sizeof(StatsGroup)] = {};
                std::memcpy(entry, &group_id, sizeof(group_id));
                std::memcpy(entry + sizeof(group_id), &shard,
                            sizeof(shard));
                hub.writer().writeStatsEntryFromPlan(
                    entry, sizeof(StatsGroup));
            }
        }
    }
    return Ok();
}

Status CheckpointReplay::runSnapshot(ResourceHub& hub, const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    if (hub.handles().size() > 0) {
        std::uint32_t count = as_u32(hub.handles().size());
        hub.writer().begin(CheckpointBeginOp{1, count, 16});
        auto entries = hub.handles().snapshotEntries();
        hub.writer().writeHandleTableFromExpanded(entries,
            CheckpointWriteHandleTableOp{1, count, 16});
    }
    return Ok();
}

Status CheckpointReplay::runWithPins(ResourceHub& hub, const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    std::uint32_t count = as_u32(hub.pins().size());
    hub.writer().begin(CheckpointBeginOp{2, count, 32});
    hub.writer().writeHandleTable(CheckpointWriteHandleTableOp{
        2, count, 32});
    return Ok();
}

Status PinReplay::runSnapshotRelease(ResourceHub& hub, const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    hub.pins().releaseAll();
    return Ok();
}

Status DeferredReplay::runCompactFlush(ResourceHub& hub,
                                       const OpStream& ops) {
    OpStream push_only;
    OpStream compact_flush_only;
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        if (op.kind == OpKind::DeferPush) {
            push_only.append(op.kind, op.a, op.b, op.c, op.d);
        } else if (op.kind == OpKind::DeferCompact ||
                   op.kind == OpKind::DeferFlush) {
            compact_flush_only.append(op.kind, op.a, op.b, op.c, op.d);
        }
    }
    Status s = hub.populateFromOps(push_only);
    if (!s) {
        return s;
    }
    if (hub.deferred().size() > 0) {
        const std::uint8_t* front = hub.deferred().peekFrontPayload();
        std::uint32_t front_size = hub.deferred().peekFrontSize();
        if (front != nullptr && front_size > 0) {
            hub.pins().retainDeferredView(front, front_size);
        }
    }
    s = hub.populateFromOps(compact_flush_only);
    if (!s) {
        return s;
    }
    hub.pins().readDeferredViews();
    return Ok();
}

Status CompactReplay::runPoolShardCompact(ResourceHub& hub,
                                            SlabCache& pool, ShardMap& shards,
                                            const OpStream& ops) {
    (void)pool;
    OpStream pool_only;
    OpStream plan_only;
    OpStream commit_only;
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::MigratePlan:
            plan_only.append(op.kind, op.a, op.b, op.c, op.d);
            break;
        case OpKind::MigrateCommit:
            commit_only.append(op.kind, op.a, op.b, 0, 0);
            break;
        default:
            pool_only.append(op.kind, op.a, op.b, op.c, op.d);
            break;
        }
    }
    hub.populateFromOps(pool_only);
    shards.attachShard(0, &hub.pool());
    shards.attachShard(1, &hub.pool());
    shards.registerClassMembership(0, 0);
    shards.registerClassMembership(1, 0);
    hub.migrator().apply(hub.pool(), hub.pool(), plan_only);
    shards.rebalance(0, 1);
    hub.migrator().apply(hub.pool(), hub.pool(), commit_only);
    return Ok();
}

Status ShardReplay::runRebalance(ResourceHub& hub, SlabCache& a,
                                   SlabCache& b, ShardMap& shards,
                                   const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    shards.attachShard(0, &a);
    shards.attachShard(1, &b);
    std::uint32_t shard0_classes = 0;
    std::uint32_t shard1_classes = 0;
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        if (op.kind == OpKind::PoolAlloc && op.b == 0) {
            if (shard0_classes < 8) {
                shards.registerClassMembership(0, op.a);
                ++shard0_classes;
            }
        } else if (op.kind == OpKind::PoolAlloc && op.b == 1) {
            if (shard1_classes < 8) {
                shards.registerClassMembership(1, op.a);
                ++shard1_classes;
            }
        } else if (op.kind == OpKind::StatsSample) {
            shards.registerClassMembership(0, op.b);
            shards.registerClassMembership(1, op.b);
        }
    }
    shards.setStatsCallback(&hub.stats());
    hub.stats().prepareMoveBuffer(
        static_cast<std::uint32_t>(shards.shardCount()));
    shards.rebalance(0, 1);
    return Ok();
}

Status GuardReplay::runHandleGuardCheck(ResourceHub& hub,
                                        const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    for (std::size_t i = 1; i < hub.guards().size() + 1; ++i) {
        std::uint32_t id = as_u32(i);
        hub.guards().verifyRedzone(SpanGuardVerifyOp{id});
    }
    return Ok();
}

Status BatchReplay::runCommitCheckpoint(ResourceHub& hub,
                                          const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    for (std::uint32_t b = 1; b <= 16; ++b) {
        hub.batches().commit(BatchCommitOp{b});
    }
    hub.writer().writeBatchSection(hub.batches(), hub.arena());
    return Ok();
}

Status TeardownReplay::runPinnedEvictDestroy(ResourceHub& hub,
                                              const OpStream& ops) {
    Status s = applyToHub(hub, ops);
    if (!s) {
        return s;
    }
    std::uint32_t target = 0;
    bool has_evict = false;
    for (std::size_t i = 0; i < ops.size(); ++i) {
        if (ops[i].kind == OpKind::HubEvict) {
            target = ops[i].a;
            has_evict = true;
            break;
        }
    }
    if (!has_evict) {
        target = 1;
        if (target >= hub.bucketCount()) {
            target = 0;
        }
    }
    const ResourceBucket& bucket = hub.bucket(target);
    if (bucket.summary != nullptr) {
        hub.pins().pinSummaryView(bucket.id, bucket.summary,
                                    bucket.summary_bytes);
    }
    hub.evictBucket(bucket.id, false);
    return Ok();
}

}  // namespace memforge
