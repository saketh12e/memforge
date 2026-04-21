#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/batch_allocator.hpp>

namespace {
void test_batch_stage_commit() {
    using namespace memforge;
    BatchAllocator b;
    b.init();
    b.stage(BatchStageOp{1, 8, 16});
    b.commit(BatchCommitOp{1});
    assert(b.pending() == 0);
    b.shutdown();
}

void test_batch_invalid_id() {
    using namespace memforge;
    BatchAllocator b;
    b.init();
    assert(!b.stage(BatchStageOp{0, 8, 16}).ok());
    b.shutdown();
}

void test_batch_zero_count() {
    using namespace memforge;
    BatchAllocator b;
    b.init();
    assert(!b.stage(BatchStageOp{1, 0, 16}).ok());
    assert(!b.stage(BatchStageOp{1, 8, 0}).ok());
    b.shutdown();
}

void test_batch_capacity() {
    using namespace memforge;
    BatchAllocator b;
    b.init();
    for (std::uint32_t i = 0; i < kMaxBatchEntries; ++i) {
        assert(b.stage(BatchStageOp{i + 1, 1, 16}).ok());
    }
    assert(!b.stage(BatchStageOp{kMaxBatchEntries + 1, 1, 16}).ok());
    b.shutdown();
}

void test_batch_commit_empty() {
    using namespace memforge;
    BatchAllocator b;
    b.init();
    assert(b.commit(BatchCommitOp{1}).ok());
    b.shutdown();
}

void test_batch_apply() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::BatchStage, 1, 8, 16, 0);
    ops.append(OpKind::BatchCommit, 1, 0, 0, 0);
    BatchAllocator b;
    b.init();
    b.apply(ops);
    b.shutdown();
}
}  // namespace

// static memforge_test::Register r1("batch.stage_commit", &test_batch_stage_commit);
// static memforge_test::Register r2("batch.invalid_id", &test_batch_invalid_id);
// static memforge_test::Register r3("batch.zero_count", &test_batch_zero_count);
// static memforge_test::Register r4("batch.capacity", &test_batch_capacity);
// static memforge_test::Register r5("batch.commit_empty", &test_batch_commit_empty);
// static memforge_test::Register r6("batch.apply", &test_batch_apply);
