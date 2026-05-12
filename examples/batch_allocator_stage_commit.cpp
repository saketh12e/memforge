// Example: batch allocator stage/commit.
//
// Stages multiple batches with different counts and strides, then commits
// them all in one write.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    BatchAllocator batches;
    batches.init();

    batches.stage(BatchStageOp{1, 8, 16});
    batches.stage(BatchStageOp{2, 16, 32});
    batches.stage(BatchStageOp{3, 4, 64});
    std::printf("staged: %zu\n", batches.pending());
    batches.commit(BatchCommitOp{1});
    std::printf("after commit: pending=%zu slab_bytes=%u\n",
                batches.pending(), batches.currentSlabBytes());
    batches.shutdown();
    return 0;
}
