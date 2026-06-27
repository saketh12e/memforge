#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/api.hpp>

// Stress tests: lots of op streams, lots of module combinations.

namespace {
void test_stress_100_arena_appends() {
    using namespace memforge;
    ArenaChain chain;
    chain.init();
    for (std::uint32_t i = 0; i < 100; ++i) {
        chain.appendBlock(ArenaAppendOp{i + 1, 32, 8, i});
    }
    chain.finalizeBlocks();
    assert(chain.blockCount() == 100);
    chain.shutdown();
}

void test_stress_100_pool_allocs() {
    using namespace memforge;
    SlabCache cache;
    cache.init();
    std::vector<SlabSlot> slots;
    for (std::uint32_t i = 0; i < 100; ++i) {
        SlabSlot s{};
        cache.alloc(i % kNumSlabClasses, s);
        slots.push_back(s);
    }
    assert(slots.size() == 100);
    cache.shutdown();
}

void test_stress_100_handle_inserts() {
    using namespace memforge;
    HandleTable table;
    table.init();
    std::vector<std::uint32_t> handles;
    for (std::uint32_t i = 0; i < 100; ++i) {
        std::uint32_t h = 0;
        table.insert(HandleInsertOp{(i % 32) + 1, i + 1}, h);
        handles.push_back(h);
    }
    assert(handles.size() == 100);
    table.shutdown();
}

void test_stress_100_epoch_enqueues() {
    using namespace memforge;
    EpochQueue queue;
    queue.init();
    for (std::uint32_t i = 0; i < 100; ++i) {
        queue.enqueue(EpochEnqueueOp{i + 1, 16});
    }
    assert(queue.pending() == 100);
    queue.shutdown();
}

void test_stress_100_pin() {
    using namespace memforge;
    PinRegistry pins;
    pins.init();
    for (std::uint32_t i = 0; i < 100; ++i) {
        pins.pin(PinOp{i + 1, 0});
    }
    assert(pins.size() == 100);
    pins.shutdown();
}

void test_stress_100_defer_push() {
    using namespace memforge;
    DeferredFreeList list;
    list.init();
    for (std::uint32_t i = 0; i < 100; ++i) {
        list.push(DeferPushOp{i + 1, 16});
    }
    assert(list.size() == 100);
    list.shutdown();
}

void test_stress_100_stats_samples() {
    using namespace memforge;
    StatsAggregator agg;
    agg.init();
    int ok = 0;
    for (std::uint32_t i = 0; i < 100; ++i) {
        if (agg.sample(StatsSampleOp{i + 1, (i % kStatsGroups) + 1, 4}).ok()) {
            ++ok;
        }
    }
    assert(ok > 0);
    agg.shutdown();
}

void test_stress_100_guard_attaches() {
    using namespace memforge;
    SpanGuard g;
    g.init();
    for (std::uint32_t i = 1; i <= 100; ++i) {
        g.attach(SpanGuardAttachOp{i, (i % kSpanGuardClasses), 0});
    }
    assert(g.size() == 100);
    g.shutdown();
}

void test_stress_100_batch_stages() {
    using namespace memforge;
    BatchAllocator b;
    b.init();
    for (std::uint32_t i = 0; i < 100; ++i) {
        b.stage(BatchStageOp{i + 1, 1, 8});
    }
    assert(b.pending() == 100);
    b.shutdown();
}

void test_stress_100_scope_begins() {
    using namespace memforge;
    TxScope scope;
    scope.init();
    for (std::uint32_t i = 1; i <= 100; ++i) {
        scope.begin(i);
    }
    assert(scope.scopeCount() == 100);
    scope.shutdown();
}

void test_stress_hub_100_lifecycles() {
    using namespace memforge;
    for (int round = 0; round < 50; ++round) {
        ResourceHub hub;
        hub.init();
        OpStream ops;
        ops.appendArenaAppend(1, 16, 8, 1);
        hub.applyOps(ops);
        hub.destroy();
    }
}

void test_stress_hub_long_opstream() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    for (std::uint32_t i = 0; i < 256; ++i) {
        ops.appendArenaAppend(i + 1, 32, 8, i);
        ops.append(OpKind::PoolAlloc, i % 6, 0, 0, 0);
        ops.append(OpKind::HandleInsert, (i % 64) + 1, i, 0, 0);
        ops.append(OpKind::Pin, i + 1, 0, 0, 0);
    }
    hub.applyOps(ops);
    assert(hub.arena().blockCount() == 256);
    assert(hub.pins().size() == 256);
    hub.destroy();
}

void test_stress_arena_alignment_variations() {
    using namespace memforge;
    ArenaChain chain;
    chain.init();
    const std::uint32_t alignments[] = {1, 2, 4, 8, 16, 32, 64, 128};
    for (std::uint32_t i = 0; i < 32; ++i) {
        std::uint32_t align = alignments[i % 8];
        std::uint32_t bytes = (i * 7) + 13;
        chain.appendBlock(ArenaAppendOp{i + 1, bytes, align, i});
    }
    chain.finalizeBlocks();
    assert(chain.blockCount() == 32);
    chain.shutdown();
}

void test_stress_pool_fragmentation_cycles() {
    using namespace memforge;
    SlabCache cache;
    cache.init();
    for (std::uint32_t cycle = 0; cycle < 8; ++cycle) {
        std::vector<SlabSlot> slots;
        for (std::uint32_t i = 0; i < 32; ++i) {
            SlabSlot s{};
            cache.alloc(cycle % 6, s);
            slots.push_back(s);
        }
        for (std::uint32_t i = 0; i < 16; ++i) {
            cache.freeSlot(cycle % 6, slots[i].id);
        }
    }
    cache.shutdown();
}

void test_stress_handle_churn_heavy() {
    using namespace memforge;
    HandleTable table;
    table.init();
    std::vector<std::uint32_t> handles(64, 0);
    for (std::uint32_t round = 0; round < 16; ++round) {
        for (std::uint32_t i = 1; i <= 64; ++i) {
            table.insert(HandleInsertOp{i, round * 64 + i}, handles[i - 1]);
        }
    }
    table.shutdown();
}
}  // namespace

static memforge_test::Register r1("stress.100_arena_appends", &test_stress_100_arena_appends);
static memforge_test::Register r2("stress.100_pool_allocs", &test_stress_100_pool_allocs);
static memforge_test::Register r3("stress.100_handle_inserts", &test_stress_100_handle_inserts);
static memforge_test::Register r4("stress.100_epoch_enqueues", &test_stress_100_epoch_enqueues);
static memforge_test::Register r5("stress.100_pin", &test_stress_100_pin);
static memforge_test::Register r6("stress.100_defer_push", &test_stress_100_defer_push);
static memforge_test::Register r7("stress.100_stats_samples", &test_stress_100_stats_samples);
static memforge_test::Register r8("stress.100_guard_attaches", &test_stress_100_guard_attaches);
static memforge_test::Register r9("stress.100_batch_stages", &test_stress_100_batch_stages);
static memforge_test::Register r10("stress.100_scope_begins", &test_stress_100_scope_begins);
static memforge_test::Register r11("stress.hub_100_lifecycles", &test_stress_hub_100_lifecycles);
static memforge_test::Register r12("stress.hub_long_opstream", &test_stress_hub_long_opstream);
static memforge_test::Register r13("stress.arena_alignment_variations", &test_stress_arena_alignment_variations);
static memforge_test::Register r14("stress.pool_fragmentation_cycles", &test_stress_pool_fragmentation_cycles);
static memforge_test::Register r15("stress.handle_churn_heavy", &test_stress_handle_churn_heavy);
