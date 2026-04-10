// MemForge - sub-block free list allocator.
//
// Provides a free list for sub-block allocations within an arena block.
// This is a utility allocator that complements the bump allocator.

#ifndef MEMFORGE_FREE_LIST_HPP
#define MEMFORGE_FREE_LIST_HPP

#include <cstddef>
#include <cstdint>

#include <memforge/error.hpp>

namespace memforge {

class FreeListAllocator {
public:
    FreeListAllocator() = default;
    ~FreeListAllocator();

    FreeListAllocator(const FreeListAllocator&) = delete;
    FreeListAllocator& operator=(const FreeListAllocator&) = delete;

    Status init(std::uint32_t slot_count, std::uint32_t slot_bytes);
    void shutdown() noexcept;

    Status alloc(std::uint8_t*& out);
    Status free_slot(std::uint8_t* slot);

    std::size_t free_count() const noexcept { return free_.size(); }
    std::uint32_t slot_bytes() const noexcept { return slot_bytes_; }

private:
    std::uint8_t* base_{nullptr};
    std::vector<std::uint8_t*> free_;
    std::uint32_t slot_bytes_{0};
    std::uint32_t slot_count_{0};
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_FREE_LIST_HPP
