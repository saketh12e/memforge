// MemForge - batch allocator.
//
// Stages many same-stride allocations into one contiguous write at commit().
// stage() enqueues an entry; commit() materializes the batch.

#ifndef MEMFORGE_BATCH_ALLOCATOR_HPP
#define MEMFORGE_BATCH_ALLOCATOR_HPP

#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

inline constexpr std::uint32_t kMaxBatchEntries = 8192;

struct BatchEntry {
    std::uint32_t id{0};
    std::uint32_t stride{0};
    std::uint32_t count{0};
};

class BatchAllocator {
public:
    BatchAllocator();
    ~BatchAllocator();

    BatchAllocator(const BatchAllocator&) = delete;
    BatchAllocator& operator=(const BatchAllocator&) = delete;

    Status init();
    void shutdown() noexcept;

    Status stage(const BatchStageOp& op);
    Status commit(const BatchCommitOp& op);

    std::size_t pending() const noexcept { return staged_.size(); }

    std::uint32_t currentSlabBytes() const noexcept;
    static std::uint32_t headerBytesPerBatch() noexcept;
    static std::uint32_t trailerBytesPerBatch() noexcept;

    Status apply(const OpStream& ops);

private:
    std::vector<BatchEntry> staged_;
    std::uint8_t* slab_{nullptr};
    std::uint32_t slab_bytes_{0};
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_BATCH_ALLOCATOR_HPP
