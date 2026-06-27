#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/api.hpp>

// Stress test: drive the hub through a long mixed op stream and verify the
// public counters stay sane.
namespace {
void test_stress_long_opstream() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();

    OpStream ops;
    for (std::uint32_t i = 0; i < 32; ++i) {
        ops.appendArenaAppend(i + 1, 32 + i * 4, 8, i);
        ops.append(OpKind::PoolAlloc, i % kNumSlabClasses, 0, 0, 0);
        ops.append(OpKind::HandleInsert, (i % 64) + 1, i, 0, 0);
        ops.append(OpKind::EpochEnqueue, i + 1, 16, 0, 0);
    }
    ops.append(OpKind::EpochTick, 4, 0, 0, 0);
    ops.append(OpKind::EpochDrain, 64, 0, 0, 0);

    hub.applyOps(ops);
    assert(hub.arena().blockCount() == 32);
    hub.destroy();
}

void test_stress_mixed_workflow() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.appendArenaAppend(1, 64, 8, 1);
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    ops.append(OpKind::PoolAlloc, 1, 0, 0, 0);
    ops.append(OpKind::HandleInsert, 1, 1, 0, 0);
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    ops.append(OpKind::TxBegin, 1, 0, 0, 0);
    ops.append(OpKind::TxRecord, 1, 64, 0, 0);
    ops.append(OpKind::TxCommit, 1, 0, 0, 0);
    ops.append(OpKind::StatsSample, 1, 1, 4, 0);
    ops.append(OpKind::BatchStage, 1, 16, 32, 0);
    ops.append(OpKind::BatchCommit, 1, 0, 0, 0);
    ops.append(OpKind::SpanGuardAttach, 1, 2, 0, 0);
    ops.append(OpKind::SpanGuardVerify, 1, 0, 0, 0);
    ops.append(OpKind::CheckpointBegin, 1, 4, 16, 0);
    ops.append(OpKind::CheckpointWriteHandleTable, 1, 4, 16, 0);
    hub.applyOps(ops);
    assert(hub.bucketCount() == 8);
    HubMetrics m = hub.collectMetrics();
    assert(m.arena.blocks == 1);
    assert(m.pool.classes == kNumSlabClasses);
    hub.destroy();
}

void test_stress_repeated_lifecycles() {
    using namespace memforge;
    for (int round = 0; round < 4; ++round) {
        ResourceHub hub;
        hub.init();
        OpStream ops;
        ops.appendArenaAppend(1, 16, 8, 1);
        hub.applyOps(ops);
        hub.destroy();
    }
}

void test_stress_handle_churn() {
    using namespace memforge;
    HandleTable table;
    table.init();
    for (std::uint32_t i = 0; i < 64; ++i) {
        std::uint32_t h = 0;
        table.insert(HandleInsertOp{(i % 16) + 1, i}, h);
    }
    assert(table.size() >= 1);
    table.shutdown();
}
}  // namespace

static memforge_test::Register r1("stress.long_opstream", &test_stress_long_opstream);
static memforge_test::Register r2("stress.mixed_workflow", &test_stress_mixed_workflow);
static memforge_test::Register r3("stress.repeated_lifecycles", &test_stress_repeated_lifecycles);
static memforge_test::Register r4("stress.handle_churn", &test_stress_handle_churn);
