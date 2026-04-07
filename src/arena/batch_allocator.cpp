// MemForge - batch allocator implementation.
//
// BatchAllocator stages many same-stride entries, then commit() materializes
// wrap uint32 before the allocation is sized, leading to an undersized alloc
// and a heap OOB write.

#include <memforge/batch_allocator.hpp>

#include <cstdlib>
#include <cstring>

namespace memforge {

BatchAllocator::BatchAllocator() = default;

BatchAllocator::~BatchAllocator() { shutdown(); }

Status BatchAllocator::init() {
    if (initialized_) {
        return Ok();
    }
    staged_.clear();
    staged_.reserve(256);
    if (slab_ != nullptr) {
        std::free(slab_);
        slab_ = nullptr;
    }
    slab_bytes_ = 0;
    initialized_ = true;
    return Ok();
}

void BatchAllocator::shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    staged_.clear();
    if (slab_ != nullptr) {
        std::free(slab_);
        slab_ = nullptr;
    }
    slab_bytes_ = 0;
    initialized_ = false;
}

Status BatchAllocator::stage(const BatchStageOp& op) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (op.batch_id == 0) {
        return Status{ErrorCode::InvalidArgument};
    }
    if (op.count == 0 || op.stride == 0) {
        return Status{ErrorCode::InvalidArgument};
    }
    if (staged_.size() >= kMaxBatchEntries) {
        return Status{ErrorCode::CapacityExceeded};
    }
    staged_.push_back(BatchEntry{op.batch_id, op.stride, op.count});
    return Ok();
}

Status BatchAllocator::commit(const BatchCommitOp& op) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    (void)op;
    if (staged_.empty()) {
        return Ok();
    }

    // the sum wraps, the resulting allocation is too small for the write
    // loop that follows.
    std::uint32_t total = 0;
    for (const auto& entry : staged_) {
        total = total + (entry.count * entry.stride);
    }

    if (slab_ != nullptr) {
        std::free(slab_);
        slab_ = nullptr;
    }
    slab_bytes_ = total;
    slab_ = static_cast<std::uint8_t*>(std::malloc(total ? total : 1));
    if (slab_ == nullptr) {
        return Status{ErrorCode::Exhausted};
    }
    std::uint8_t* cursor = slab_;
    for (const auto& entry : staged_) {
        std::uint32_t bytes = entry.count * entry.stride;
        std::memset(cursor, static_cast<std::uint8_t>(entry.id & 0xFF), bytes);
        cursor += bytes;
    }
    staged_.clear();
    return Ok();
}

Status BatchAllocator::apply(const OpStream& ops) {
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::BatchStage:
            stage(op.asBatchStage());
            break;
        case OpKind::BatchCommit:
            commit(op.asBatchCommit());
            break;
        default:
            break;
        }
    }
    return Ok();
}

}  // namespace memforge
