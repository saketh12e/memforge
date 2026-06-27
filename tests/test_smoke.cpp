#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/api.hpp>

// End-to-end smoke tests that exercise the ResourceHub with realistic op
// streams. These tests do NOT trigger the planted bugs; they verify the
// happy path for each module.

namespace {
void test_smoke_arena_only() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    for (std::uint32_t i = 0; i < 4; ++i) {
        ops.appendArenaAppend(i + 1, 16, 8, i);
    }
    hub.applyOps(ops);
    assert(hub.arena().blockCount() == 4);
    hub.destroy();
}

void test_smoke_pool_only() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    for (std::uint32_t i = 0; i < 8; ++i) {
        ops.append(OpKind::PoolAlloc, i % 6, 0, 0, 0);
    }
    hub.applyOps(ops);
    assert(hub.pool().totalSlabs() >= 1);
    hub.destroy();
}

void test_smoke_handle_only() {
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

void test_smoke_epoch_only() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::EpochEnqueue, 1, 16, 0, 0);
    ops.append(OpKind::EpochTick, 1, 0, 0, 0);
    hub.applyOps(ops);
    assert(hub.epoch().epoch() == 1);
    hub.destroy();
}

void test_smoke_scope_only() {
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

void test_smoke_pin_only() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    hub.applyOps(ops);
    assert(hub.pins().size() == 1);
    hub.destroy();
}

void test_smoke_defer_only() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::DeferPush, 1, 16, 0, 0);
    hub.applyOps(ops);
    assert(hub.deferred().size() == 1);
    hub.destroy();
}

void test_smoke_stats_only() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::StatsSample, 1, 1, 4, 0);
    hub.applyOps(ops);
    assert(hub.stats().sampleCount() == 1);
    hub.destroy();
}

void test_smoke_checkpoint_only() {
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

void test_smoke_guard_only() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::SpanGuardAttach, 1, 2, 0, 0);
    hub.applyOps(ops);
    assert(hub.guards().size() == 1);
    hub.destroy();
}

void test_smoke_batch_only() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::BatchStage, 1, 8, 16, 0);
    ops.append(OpKind::BatchCommit, 1, 0, 0, 0);
    hub.applyOps(ops);
    assert(hub.batches().pending() == 0);
    hub.destroy();
}

void test_smoke_multiple_cycles() {
    using namespace memforge;
    for (int i = 0; i < 5; ++i) {
        ResourceHub hub;
        hub.init();
        OpStream ops;
        ops.appendArenaAppend(1, 16, 8, 1);
        ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
        ops.append(OpKind::HandleInsert, 1, 1, 0, 0);
        hub.applyOps(ops);
        hub.destroy();
    }
}

void test_smoke_empty_opstream() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    hub.applyOps(ops);
    assert(hub.bucketCount() == 8);
    hub.destroy();
}

void test_smoke_partial_opstream() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.appendArenaAppend(1, 32, 8, 1);
    hub.applyOps(ops);
    assert(hub.arena().blockCount() == 1);
    hub.destroy();
}

void test_smoke_double_apply() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    hub.applyOps(ops);
    OpStream more;
    more.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    hub.applyOps(more);
    assert(hub.arena().blockCount() == 1);
    assert(hub.pool().totalSlabs() >= 1);
    hub.destroy();
}

void test_smoke_apply_then_destroy() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.appendArenaAppend(1, 32, 8, 1);
    hub.applyOps(ops);
    hub.destroy();
    hub.init();
    OpStream ops2;
    ops2.appendArenaAppend(1, 64, 8, 1);
    hub.applyOps(ops2);
    hub.destroy();
}

void test_smoke_metrics_collection() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.appendArenaAppend(1, 32, 8, 1);
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    ops.append(OpKind::HandleInsert, 1, 1, 0, 0);
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    hub.applyOps(ops);
    HubMetrics m = hub.collectMetrics();
    assert(m.arena.blocks == 1);
    assert(m.buckets == 8);
    hub.destroy();
}

void test_smoke_zero_byte_arena() {
    using namespace memforge;
    ArenaChain chain;
    chain.init();
    assert(chain.finalizeBlocks().ok());
    assert(chain.finalizedBytes() == 0);
    chain.shutdown();
}
}  // namespace

static memforge_test::Register r1("smoke.arena_only", &test_smoke_arena_only);
static memforge_test::Register r2("smoke.pool_only", &test_smoke_pool_only);
static memforge_test::Register r3("smoke.handle_only", &test_smoke_handle_only);
static memforge_test::Register r4("smoke.epoch_only", &test_smoke_epoch_only);
static memforge_test::Register r5("smoke.scope_only", &test_smoke_scope_only);
static memforge_test::Register r6("smoke.pin_only", &test_smoke_pin_only);
static memforge_test::Register r7("smoke.defer_only", &test_smoke_defer_only);
static memforge_test::Register r8("smoke.stats_only", &test_smoke_stats_only);
static memforge_test::Register r9("smoke.checkpoint_only", &test_smoke_checkpoint_only);
static memforge_test::Register r10("smoke.guard_only", &test_smoke_guard_only);
static memforge_test::Register r11("smoke.batch_only", &test_smoke_batch_only);
static memforge_test::Register r12("smoke.multiple_cycles", &test_smoke_multiple_cycles);
static memforge_test::Register r13("smoke.empty_opstream", &test_smoke_empty_opstream);
static memforge_test::Register r14("smoke.partial_opstream", &test_smoke_partial_opstream);
static memforge_test::Register r15("smoke.double_apply", &test_smoke_double_apply);
static memforge_test::Register r16("smoke.apply_then_destroy", &test_smoke_apply_then_destroy);
static memforge_test::Register r17("smoke.metrics_collection", &test_smoke_metrics_collection);
static memforge_test::Register r18("smoke.zero_byte_arena", &test_smoke_zero_byte_arena);
