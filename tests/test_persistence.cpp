#include <cassert>
#include <cstdio>
#include <vector>

#include "test_framework.hpp"
#include <memforge/api.hpp>

// Tests that exercise ResourceHub across multiple lifecycles in the same
// process, validating shutdown ordering and resource reclamation.

namespace {
void test_hub_arena_persistence() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.appendArenaAppend(1, 32, 8, 1);
    hub.applyOps(ops);
    std::uint8_t* slab1 = hub.arena().finalizedSlab();
    assert(slab1 != nullptr);
    assert(hub.arena().finalizedBytes() >= 32);
    hub.destroy();
}

void test_hub_pool_persistence() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    for (std::uint32_t i = 0; i < 8; ++i) {
        ops.append(OpKind::PoolAlloc, i % kNumSlabClasses, 0, 0, 0);
    }
    hub.applyOps(ops);
    assert(hub.pool().totalSlabs() >= 1);
    hub.destroy();
}

void test_hub_handle_persistence() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    for (std::uint32_t i = 0; i < 4; ++i) {
        ops.append(OpKind::HandleInsert, i + 1, i + 1, 0, 0);
    }
    hub.applyOps(ops);
    assert(hub.handles().size() == 4);
    hub.destroy();
}

void test_hub_epoch_persistence() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::EpochEnqueue, 1, 16, 0, 0);
    ops.append(OpKind::EpochTick, 2, 0, 0, 0);
    ops.append(OpKind::EpochDrain, 8, 0, 0, 0);
    hub.applyOps(ops);
    assert(hub.epoch().epoch() == 2);
    hub.destroy();
}

void test_hub_scope_persistence() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::TxBegin, 1, 0, 0, 0);
    ops.append(OpKind::TxCommit, 1, 0, 0, 0);
    hub.applyOps(ops);
    assert(hub.scope().scopeCount() == 1);
    hub.destroy();
}

void test_hub_pin_persistence() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    hub.applyOps(ops);
    assert(hub.pins().size() == 1);
    hub.destroy();
}

void test_hub_defer_persistence() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::DeferPush, 1, 16, 0, 0);
    hub.applyOps(ops);
    assert(hub.deferred().size() == 1);
    hub.destroy();
}

void test_hub_stats_persistence() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::StatsSample, 1, 1, 4, 0);
    hub.applyOps(ops);
    assert(hub.stats().sampleCount() == 1);
    hub.destroy();
}

void test_hub_checkpoint_persistence() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::CheckpointBegin, 1, 4, 16, 0);
    ops.append(OpKind::CheckpointWriteHandleTable, 1, 4, 16, 0);
    hub.applyOps(ops);
    assert(hub.writer().byteSize() > 0);
    hub.destroy();
}

void test_hub_guard_persistence() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::SpanGuardAttach, 1, 2, 0, 0);
    hub.applyOps(ops);
    assert(hub.guards().size() == 1);
    hub.destroy();
}

void test_hub_batch_persistence() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::BatchStage, 1, 8, 16, 0);
    hub.applyOps(ops);
    assert(hub.batches().pending() == 1);
    ops.append(OpKind::BatchCommit, 1, 0, 0, 0);
    hub.applyOps(ops);
    assert(hub.batches().pending() == 0);
    hub.destroy();
}

void test_hub_shard_persistence() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    hub.shards().init();
    SlabCache a;
    SlabCache b;
    a.init();
    b.init();
    hub.shards().attachShard(0, &a);
    hub.shards().attachShard(1, &b);
    OpStream ops;
    ops.append(OpKind::ShardRebalance, 0, 1, 0, 0);
    hub.applyOps(ops);
    hub.shards().shutdown();
    a.shutdown();
    b.shutdown();
    hub.destroy();
}

void test_hub_migrator_persistence() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::MigratePlan, 0, 0, 4, 32);
    hub.applyOps(ops);
    assert(hub.migrator().planSize() == 4);
    hub.destroy();
}

void test_hub_evict_keep_summary() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    hub.evictBucket(1, true);
    assert(hub.bucket(0).id == 1);
    hub.destroy();
}

void test_hub_evict_drop_summary() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    hub.evictBucket(1, false);
    hub.destroy();
}

void test_hub_metrics_after_arena() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.appendArenaAppend(1, 64, 8, 1);
    hub.applyOps(ops);
    HubMetrics m = hub.collectMetrics();
    assert(m.arena.total_bytes >= 64);
    assert(m.buckets == 8);
    hub.destroy();
}

void test_hub_metrics_after_pool() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    ops.append(OpKind::PoolAlloc, 1, 0, 0, 0);
    ops.append(OpKind::PoolAlloc, 2, 0, 0, 0);
    hub.applyOps(ops);
    HubMetrics m = hub.collectMetrics();
    assert(m.pool.slabs >= 1);
    hub.destroy();
}

void test_hub_metrics_after_handle() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    for (std::uint32_t i = 0; i < 4; ++i) {
        ops.append(OpKind::HandleInsert, i + 1, i + 1, 0, 0);
    }
    hub.applyOps(ops);
    HubMetrics m = hub.collectMetrics();
    assert(m.handles.live == 4);
    hub.destroy();
}

void test_hub_metrics_after_pin() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    hub.applyOps(ops);
    HubMetrics m = hub.collectMetrics();
    assert(m.pins.entries == 1);
    hub.destroy();
}

void test_hub_metrics_after_checkpoint() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::CheckpointBegin, 1, 4, 16, 0);
    ops.append(OpKind::CheckpointWriteHandleTable, 1, 4, 16, 0);
    hub.applyOps(ops);
    HubMetrics m = hub.collectMetrics();
    assert(m.checkpoint.bytes > 0);
    hub.destroy();
}

void test_hub_double_destroy() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    hub.destroy();
    hub.destroy();
}
}  // namespace

static memforge_test::Register r1("persistence.arena", &test_hub_arena_persistence);
static memforge_test::Register r2("persistence.pool", &test_hub_pool_persistence);
static memforge_test::Register r3("persistence.handle", &test_hub_handle_persistence);
static memforge_test::Register r4("persistence.epoch", &test_hub_epoch_persistence);
static memforge_test::Register r5("persistence.scope", &test_hub_scope_persistence);
static memforge_test::Register r6("persistence.pin", &test_hub_pin_persistence);
static memforge_test::Register r7("persistence.defer", &test_hub_defer_persistence);
static memforge_test::Register r8("persistence.stats", &test_hub_stats_persistence);
static memforge_test::Register r9("persistence.checkpoint", &test_hub_checkpoint_persistence);
static memforge_test::Register r10("persistence.guard", &test_hub_guard_persistence);
static memforge_test::Register r11("persistence.batch", &test_hub_batch_persistence);
static memforge_test::Register r12("persistence.shard", &test_hub_shard_persistence);
static memforge_test::Register r13("persistence.migrator", &test_hub_migrator_persistence);
static memforge_test::Register r14("persistence.evict_keep", &test_hub_evict_keep_summary);
static memforge_test::Register r15("persistence.evict_drop", &test_hub_evict_drop_summary);
static memforge_test::Register r16("persistence.metrics_arena", &test_hub_metrics_after_arena);
static memforge_test::Register r17("persistence.metrics_pool", &test_hub_metrics_after_pool);
static memforge_test::Register r18("persistence.metrics_handle", &test_hub_metrics_after_handle);
static memforge_test::Register r19("persistence.metrics_pin", &test_hub_metrics_after_pin);
static memforge_test::Register r20("persistence.metrics_checkpoint", &test_hub_metrics_after_checkpoint);
static memforge_test::Register r21("persistence.double_destroy", &test_hub_double_destroy);
