#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/api.hpp>

// Integration tests that exercise multi-step scenarios across modules.

namespace {

void test_integration_full_hub_workflow() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();

    OpStream ops;
    ops.appendArenaAppend(1, 32, 8, 1);
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    ops.append(OpKind::PoolAlloc, 1, 0, 0, 0);
    ops.append(OpKind::PoolAlloc, 2, 0, 0, 0);
    ops.append(OpKind::HandleInsert, 1, 1, 0, 0);
    ops.append(OpKind::HandleInsert, 2, 2, 0, 0);
    ops.append(OpKind::HandleInsert, 3, 3, 0, 0);
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    ops.append(OpKind::StatsSample, 1, 1, 4, 0);
    ops.append(OpKind::StatsSample, 2, 2, 4, 0);
    ops.append(OpKind::BatchStage, 1, 8, 16, 0);
    ops.append(OpKind::BatchCommit, 1, 0, 0, 0);
    ops.append(OpKind::SpanGuardAttach, 1, 2, 0, 0);
    ops.append(OpKind::SpanGuardVerify, 1, 0, 0, 0);
    ops.append(OpKind::CheckpointBegin, 1, 4, 16, 0);
    ops.append(OpKind::CheckpointWriteHandleTable, 1, 4, 16, 0);
    hub.applyOps(ops);

    assert(hub.bucketCount() == 8);
    assert(hub.arena().blockCount() == 1);
    assert(hub.handles().size() == 3);
    assert(hub.guards().size() == 1);
    assert(hub.batches().pending() == 0);
    assert(hub.writer().byteSize() > 0);
    assert(hub.pins().size() == 1);
    assert(hub.stats().sampleCount() == 2);
    hub.destroy();
}

void test_integration_arena_handle() {
    using namespace memforge;
    ArenaChain arena;
    arena.init();
    HandleTable handles;
    handles.init();
    for (std::uint32_t i = 0; i < 8; ++i) {
        arena.appendBlock(ArenaAppendOp{i + 1, 32, 8, i});
        std::uint32_t h = 0;
        handles.insert(HandleInsertOp{i + 1, i + 1}, h);
    }
    arena.finalizeBlocks();
    assert(arena.blockCount() == 8);
    assert(handles.size() == 8);
    arena.shutdown();
    handles.shutdown();
}

void test_integration_pool_epoch() {
    using namespace memforge;
    SlabCache cache;
    cache.init();
    EpochQueue queue;
    queue.init();
    std::vector<SlabSlot> slots;
    for (std::uint32_t i = 0; i < 4; ++i) {
        SlabSlot s{};
        cache.alloc(0, s);
        slots.push_back(s);
        queue.enqueue(EpochEnqueueOp{i + 1, 16});
    }
    queue.tick(2);
    queue.drain(8);
    assert(cache.totalSlabs() >= 1);
    assert(queue.pending() == 0);
    cache.shutdown();
    queue.shutdown();
}

void test_integration_scope_pin() {
    using namespace memforge;
    TxScope scope;
    scope.init();
    PinRegistry pins;
    pins.init();
    scope.begin(1);
    scope.record(1, TxRecordOp{1, 32});
    pins.pin(PinOp{1, 0});
    scope.commit(1);
    assert(scope.scopeCount() == 1);
    assert(pins.size() == 1);
    pins.shutdown();
    scope.shutdown();
}

void test_integration_migrate_checkpoint() {
    using namespace memforge;
    SlabCache src;
    SlabCache dst;
    src.init();
    dst.init();
    PoolMigrator m;
    m.init();
    m.plan(src, dst, MigratePlanOp{0, 0, 4, 32});
    m.commit(src, dst);
    CheckpointWriter w;
    w.init();
    w.begin(CheckpointBeginOp{1, 4, 16});
    w.writeHandleTable(CheckpointWriteHandleTableOp{1, 4, 16});
    assert(w.byteSize() > 0);
    w.shutdown();
    m.shutdown();
    src.shutdown();
    dst.shutdown();
}

void test_integration_defer_guard() {
    using namespace memforge;
    DeferredFreeList list;
    list.init();
    SpanGuard guards;
    guards.init();
    list.push(DeferPushOp{1, 16});
    list.push(DeferPushOp{2, 16});
    guards.attach(SpanGuardAttachOp{1, 2, 0});
    guards.verifyRedzone(SpanGuardVerifyOp{1});
    list.compact();
    list.flush();
    assert(list.size() == 0);
    assert(guards.size() == 1);
    list.shutdown();
    guards.shutdown();
}

void test_integration_stats_shard() {
    using namespace memforge;
    StatsAggregator stats;
    stats.init();
    SlabCache a;
    SlabCache b;
    a.init();
    b.init();
    ShardMap shards;
    shards.init();
    shards.attachShard(0, &a);
    shards.attachShard(1, &b);
    stats.sample(StatsSampleOp{1, 1, 4});
    stats.foldMetrics(StatsFoldOp{1, 0});
    shards.rebalance(0, 1);
    assert(stats.sampleCount() == 1);
    stats.shutdown();
    shards.shutdown();
    a.shutdown();
    b.shutdown();
}

void test_integration_batch_arena() {
    using namespace memforge;
    BatchAllocator batches;
    batches.init();
    ArenaChain arena;
    arena.init();
    batches.stage(BatchStageOp{1, 8, 16});
    arena.appendBlock(ArenaAppendOp{1, 16, 8, 1});
    batches.commit(BatchCommitOp{1});
    arena.finalizeBlocks();
    assert(arena.blockCount() == 1);
    assert(batches.pending() == 0);
    arena.shutdown();
    batches.shutdown();
}

void test_integration_hub_repeated() {
    using namespace memforge;
    for (int round = 0; round < 3; ++round) {
        ResourceHub hub;
        hub.init();
        OpStream ops;
        ops.appendArenaAppend(1, 16, 8, 1);
        ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
        ops.append(OpKind::HandleInsert, 1, 1, 0, 0);
        hub.applyOps(ops);
        HubMetrics m = hub.collectMetrics();
        assert(m.arena.blocks == 1);
        hub.destroy();
    }
}

void test_integration_pool_fragmentation() {
    using namespace memforge;
    SlabCache cache;
    cache.init();
    std::vector<SlabSlot> slots;
    for (std::uint32_t i = 0; i < 16; ++i) {
        SlabSlot s{};
        cache.alloc(0, s);
        slots.push_back(s);
    }
    for (std::uint32_t i = 0; i < 8; ++i) {
        cache.freeSlot(0, slots[i].id);
    }
    double ratio = cache.fragmentationRatio();
    assert(ratio > 0.0);
    assert(ratio <= 1.0);
    cache.shutdown();
}

void test_integration_handle_packed_unpacked() {
    using namespace memforge;
    HandleTable table;
    table.init();
    for (std::uint32_t i = 1; i <= 32; ++i) {
        std::uint32_t packed = HandleTable::packedHandle(i, i);
        assert(HandleTable::slotFromPacked(packed) == i);
        assert(HandleTable::generationFromPacked(packed) == i);
        std::uint32_t out = 0;
        table.insert(HandleInsertOp{i, i}, out);
        assert(out == packed);
    }
    table.shutdown();
}

}  // namespace

static memforge_test::Register r1("integration.full_hub_workflow", &test_integration_full_hub_workflow);
static memforge_test::Register r2("integration.arena_handle", &test_integration_arena_handle);
static memforge_test::Register r3("integration.pool_epoch", &test_integration_pool_epoch);
static memforge_test::Register r4("integration.scope_pin", &test_integration_scope_pin);
static memforge_test::Register r5("integration.migrate_checkpoint", &test_integration_migrate_checkpoint);
static memforge_test::Register r6("integration.defer_guard", &test_integration_defer_guard);
static memforge_test::Register r7("integration.stats_shard", &test_integration_stats_shard);
static memforge_test::Register r8("integration.batch_arena", &test_integration_batch_arena);
static memforge_test::Register r9("integration.hub_repeated", &test_integration_hub_repeated);
static memforge_test::Register r10("integration.pool_fragmentation", &test_integration_pool_fragmentation);
static memforge_test::Register r11("integration.handle_packed_unpacked", &test_integration_handle_packed_unpacked);
