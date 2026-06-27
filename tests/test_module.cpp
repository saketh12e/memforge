#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/api.hpp>

// Tests for diagnostic helpers and module-level utility methods.

namespace {
void test_arena_alignment_waste() {
    using namespace memforge;
    ArenaChain chain;
    chain.init();
    chain.appendBlock(ArenaAppendOp{1, 17, 8, 1});
    std::uint32_t waste = chain.alignmentWaste(0);
    assert(waste == 7);  // 17 aligned to 8 = 24, 24-17=7
    chain.shutdown();
}

void test_arena_block_count() {
    using namespace memforge;
    ArenaChain chain;
    chain.init();
    assert(chain.blockCount() == 0);
    chain.appendBlock(ArenaAppendOp{1, 16, 8, 1});
    assert(chain.blockCount() == 1);
    chain.shutdown();
}

void test_pool_class_info() {
    using namespace memforge;
    SlabCache cache;
    cache.init();
    SlabClassInfo info = cache.classInfo(0);
    assert(info.size > 0);
    info = cache.classInfo(kNumSlabClasses);
    assert(info.size == 0);  // out of range
    cache.shutdown();
}

void test_pool_generation_of() {
    using namespace memforge;
    SlabCache cache;
    cache.init();
    SlabSlot s{};
    cache.alloc(0, s);
    std::uint32_t gen = cache.generationOf(0, s.id);
    assert(gen > 0);
    cache.shutdown();
}

void test_handle_size() {
    using namespace memforge;
    HandleTable table;
    table.init();
    assert(table.size() == 0);
    std::uint32_t h = 0;
    table.insert(HandleInsertOp{1, 1}, h);
    assert(table.size() == 1);
    table.shutdown();
}

void test_epoch_epoch() {
    using namespace memforge;
    EpochQueue queue;
    queue.init();
    assert(queue.epoch() == 0);
    queue.tick(3);
    assert(queue.epoch() == 3);
    queue.shutdown();
}

void test_scope_scope_count() {
    using namespace memforge;
    TxScope scope;
    scope.init();
    assert(scope.scopeCount() == 0);
    scope.begin(1);
    assert(scope.scopeCount() == 1);
    scope.shutdown();
}

void test_pin_size() {
    using namespace memforge;
    PinRegistry pins;
    pins.init();
    assert(pins.size() == 0);
    pins.pin(PinOp{1, 0});
    assert(pins.size() == 1);
    pins.shutdown();
}

void test_defer_size() {
    using namespace memforge;
    DeferredFreeList list;
    list.init();
    assert(list.size() == 0);
    list.push(DeferPushOp{1, 16});
    assert(list.size() == 1);
    list.shutdown();
}

void test_guard_size() {
    using namespace memforge;
    SpanGuard g;
    g.init();
    assert(g.size() == 0);
    g.attach(SpanGuardAttachOp{1, 2, 0});
    assert(g.size() == 1);
    g.shutdown();
}

void test_batch_pending() {
    using namespace memforge;
    BatchAllocator b;
    b.init();
    assert(b.pending() == 0);
    b.stage(BatchStageOp{1, 8, 16});
    assert(b.pending() == 1);
    b.shutdown();
}

void test_writer_byte_size() {
    using namespace memforge;
    CheckpointWriter w;
    w.init();
    assert(w.byteSize() == 0);
    w.begin(CheckpointBeginOp{1, 4, 16});
    assert(w.byteSize() > 0);
    w.shutdown();
}

void test_hub_module_access() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    (void)hub.arena();
    (void)hub.pool();
    (void)hub.handles();
    (void)hub.epoch();
    (void)hub.deferred();
    (void)hub.scope();
    (void)hub.migrator();
    (void)hub.pins();
    (void)hub.stats();
    (void)hub.writer();
    (void)hub.guards();
    (void)hub.batches();
    (void)hub.shards();
    hub.destroy();
}

void test_shard_count() {
    using namespace memforge;
    ShardMap shards;
    shards.init();
    SlabCache a;
    SlabCache b;
    a.init();
    b.init();
    shards.attachShard(0, &a);
    shards.attachShard(1, &b);
    assert(shards.shardCount() == 2);
    shards.shutdown();
    a.shutdown();
    b.shutdown();
}

void test_migrate_plan_size() {
    using namespace memforge;
    SlabCache src;
    SlabCache dst;
    src.init();
    dst.init();
    PoolMigrator m;
    m.init();
    assert(m.planSize() == 0);
    m.plan(src, dst, MigratePlanOp{0, 0, 4, 32});
    assert(m.planSize() == 4);
    m.shutdown();
    src.shutdown();
    dst.shutdown();
}

void test_hub_bucket() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    for (std::size_t i = 0; i < hub.bucketCount(); ++i) {
        const auto& b = hub.bucket(i);
        (void)b;
    }
    hub.destroy();
}
}  // namespace

static memforge_test::Register r1("module.arena_alignment_waste", &test_arena_alignment_waste);
static memforge_test::Register r2("module.arena_block_count", &test_arena_block_count);
static memforge_test::Register r3("module.pool_class_info", &test_pool_class_info);
static memforge_test::Register r4("module.pool_generation_of", &test_pool_generation_of);
static memforge_test::Register r5("module.handle_size", &test_handle_size);
static memforge_test::Register r6("module.epoch_epoch", &test_epoch_epoch);
static memforge_test::Register r7("module.scope_scope_count", &test_scope_scope_count);
static memforge_test::Register r8("module.pin_size", &test_pin_size);
static memforge_test::Register r9("module.defer_size", &test_defer_size);
static memforge_test::Register r10("module.guard_size", &test_guard_size);
static memforge_test::Register r11("module.batch_pending", &test_batch_pending);
static memforge_test::Register r12("module.writer_byte_size", &test_writer_byte_size);
static memforge_test::Register r13("module.hub_module_access", &test_hub_module_access);
static memforge_test::Register r14("module.shard_count", &test_shard_count);
static memforge_test::Register r15("module.migrate_plan_size", &test_migrate_plan_size);
static memforge_test::Register r16("module.hub_bucket", &test_hub_bucket);
