// MemForge - free list allocator implementation.

#include <memforge/free_list.hpp>

#include <cstdlib>
#include <cstring>

namespace memforge {

FreeListAllocator::~FreeListAllocator() { shutdown(); }

Status FreeListAllocator::init(std::uint32_t slot_count, std::uint32_t slot_bytes) {
    shutdown();
    slot_count_ = slot_count;
    slot_bytes_ = slot_bytes;
    if (slot_count == 0 || slot_bytes == 0) {
        return Status{ErrorCode::InvalidArgument};
    }
    std::size_t bytes = static_cast<std::size_t>(slot_count) * slot_bytes;
    base_ = static_cast<std::uint8_t*>(std::malloc(bytes));
    if (base_ == nullptr) {
        return Status{ErrorCode::Exhausted};
    }
    std::memset(base_, 0, bytes);
    free_.clear();
    free_.reserve(slot_count);
    for (std::uint32_t i = 0; i < slot_count; ++i) {
        free_.push_back(base_ + i * slot_bytes);
    }
    initialized_ = true;
    return Ok();
}

void FreeListAllocator::shutdown() noexcept {
    if (base_ != nullptr) {
        std::free(base_);
        base_ = nullptr;
    }
    free_.clear();
    slot_count_ = 0;
    slot_bytes_ = 0;
    initialized_ = false;
}

Status FreeListAllocator::alloc(std::uint8_t*& out) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (free_.empty()) {
        return Status{ErrorCode::Exhausted};
    }
    out = free_.back();
    free_.pop_back();
    return Ok();
}

Status FreeListAllocator::free_slot(std::uint8_t* slot) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (slot == nullptr) {
        return Status{ErrorCode::InvalidArgument};
    }
    if (slot < base_ || slot >= base_ + slot_count_ * slot_bytes_) {
        return Status{ErrorCode::OutOfRange};
    }
    free_.push_back(slot);
    return Ok();
}

}  // namespace memforge
