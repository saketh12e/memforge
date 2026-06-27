#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/api.hpp>
#include <memforge/op_validator.hpp>

// Tests for the op stream validator.
namespace {
void test_validator_empty() {
    using namespace memforge;
    OpStream ops;
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.total == 0);
    assert(report.valid == 0);
    assert(report.invalid == 0);
}

void test_validator_arena_append() {
    using namespace memforge;
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    ops.appendArenaAppend(2, 32, 16, 2);
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.valid == 2);
}

void test_validator_arena_align_invalid() {
    using namespace memforge;
    OpStream ops;
    ops.appendArenaAppend(1, 32, 7, 1);  // align 7 is not power of 2
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.invalid == 1);
}

void test_validator_handle_out_of_range() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::HandleInsert, 0, 1, 0, 0);  // slot 0 invalid
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.invalid == 1);
}

void test_validator_pool_alloc() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    ops.append(OpKind::PoolAlloc, 1, 0, 0, 0);
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.valid == 2);
}

void test_validator_pool_alloc_out_of_range() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::PoolAlloc, 10, 0, 0, 0);  // class 10 invalid
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.invalid == 1);
}

void test_validator_migrate() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::MigratePlan, 0, 0, 4, 32);
    ops.append(OpKind::MigrateCommit, 0, 0, 0, 0);
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.valid == 2);
}

void test_validator_migrate_same_pool() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::MigratePlan, 0, 0, 4, 32);
    ops.append(OpKind::ShardRebalance, 0, 0, 0, 0);  // same source and dest
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.invalid == 1);
}

void test_validator_pin() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    ops.append(OpKind::PinReleaseAll, 0, 0, 0, 0);
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.valid == 2);
}

void test_validator_stats() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::StatsSample, 1, 1, 4, 0);
    ops.append(OpKind::StatsFold, 1, 0, 0, 0);
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.valid == 2);
}

void test_validator_stats_group_oob() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::StatsSample, 1, 100, 4, 0);  // group 100 invalid
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.invalid == 1);
}

void test_validator_checkpoint() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::CheckpointBegin, 1, 4, 16, 0);
    ops.append(OpKind::CheckpointWriteHandleTable, 1, 4, 16, 0);
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.valid == 2);
}

void test_validator_batch() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::BatchStage, 1, 8, 16, 0);
    ops.append(OpKind::BatchCommit, 1, 0, 0, 0);
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.valid == 2);
}

void test_validator_hub_evict() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::HubEvict, 1, 1, 0, 0);
    ops.append(OpKind::HubDestroy, 0, 0, 0, 0);
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.valid == 2);
}

void test_validator_mixed() {
    using namespace memforge;
    OpStream ops;
    ops.appendArenaAppend(1, 32, 8, 1);
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    ops.append(OpKind::HandleInsert, 1, 1, 0, 0);
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    ops.append(OpKind::StatsSample, 1, 1, 4, 0);
    ops.append(OpKind::BatchStage, 1, 8, 16, 0);
    ops.append(OpKind::BatchCommit, 1, 0, 0, 0);
    OpStreamValidator v;
    auto report = v.validate(ops);
    assert(report.total == 7);
    assert(report.valid == 7);
}
}  // namespace

static memforge_test::Register r1("validator.empty", &test_validator_empty);
static memforge_test::Register r2("validator.arena_append", &test_validator_arena_append);
static memforge_test::Register r3("validator.arena_align_invalid", &test_validator_arena_align_invalid);
static memforge_test::Register r4("validator.handle_out_of_range", &test_validator_handle_out_of_range);
static memforge_test::Register r5("validator.pool_alloc", &test_validator_pool_alloc);
static memforge_test::Register r6("validator.pool_alloc_oob", &test_validator_pool_alloc_out_of_range);
static memforge_test::Register r7("validator.migrate", &test_validator_migrate);
static memforge_test::Register r8("validator.migrate_same", &test_validator_migrate_same_pool);
static memforge_test::Register r9("validator.pin", &test_validator_pin);
static memforge_test::Register r10("validator.stats", &test_validator_stats);
static memforge_test::Register r11("validator.stats_group_oob", &test_validator_stats_group_oob);
static memforge_test::Register r12("validator.checkpoint", &test_validator_checkpoint);
static memforge_test::Register r13("validator.batch", &test_validator_batch);
static memforge_test::Register r14("validator.hub_evict", &test_validator_hub_evict);
static memforge_test::Register r15("validator.mixed", &test_validator_mixed);
