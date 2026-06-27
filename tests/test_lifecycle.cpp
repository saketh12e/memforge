#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/api.hpp>

namespace {
void test_cross_module_lifecycle() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;

    ops.appendArenaAppend(1, 16, 8, 1);
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    ops.append(OpKind::PoolAlloc, 1, 0, 0, 0);
    ops.append(OpKind::HandleInsert, 1, 1, 0, 0);
    ops.append(OpKind::HandleInsert, 2, 1, 0, 0);
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    ops.append(OpKind::TxBegin, 1, 0, 0, 0);
    ops.append(OpKind::TxRecord, 1, 32, 0, 0);
    ops.append(OpKind::TxCommit, 1, 0, 0, 0);
    ops.append(OpKind::StatsSample, 1, 1, 4, 0);
    ops.append(OpKind::BatchStage, 1, 4, 16, 0);
    ops.append(OpKind::BatchCommit, 1, 0, 0, 0);
    ops.append(OpKind::SpanGuardAttach, 1, 2, 0, 0);

    hub.applyOps(ops);
    assert(hub.arena().blockCount() == 1);
    assert(hub.handles().size() == 2);
    assert(hub.batches().pending() == 0);
    hub.destroy();
}

void test_lifecycle_with_epoch() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::EpochEnqueue, 1, 16, 0, 0);
    ops.append(OpKind::EpochEnqueue, 2, 16, 0, 0);
    ops.append(OpKind::EpochTick, 2, 0, 0, 0);
    ops.append(OpKind::EpochDrain, 8, 0, 0, 0);
    hub.applyOps(ops);
    hub.destroy();
}

void test_lifecycle_with_migrate() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::MigratePlan, 0, 0, 4, 32);
    ops.append(OpKind::MigrateCommit, 0, 0, 0, 0);
    hub.applyOps(ops);
    hub.destroy();
}

void test_lifecycle_with_defer() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.append(OpKind::DeferPush, 1, 16, 0, 0);
    ops.append(OpKind::DeferPush, 2, 16, 0, 0);
    ops.append(OpKind::DeferCompact, 0, 0, 0, 0);
    ops.append(OpKind::DeferFlush, 0, 0, 0, 0);
    hub.applyOps(ops);
    hub.destroy();
}

void test_lifecycle_with_checkpoint() {
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
}  // namespace

// static memforge_test::Register r1("lifecycle.cross_module", &test_cross_module_lifecycle);
// static memforge_test::Register r2("lifecycle.with_epoch", &test_lifecycle_with_epoch);
// static memforge_test::Register r3("lifecycle.with_migrate", &test_lifecycle_with_migrate);
// static memforge_test::Register r4("lifecycle.with_defer", &test_lifecycle_with_defer);
// static memforge_test::Register r5("lifecycle.with_checkpoint", &test_lifecycle_with_checkpoint);
