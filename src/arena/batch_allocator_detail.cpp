// MemForge - batch allocator deep implementation.

#include <memforge/batch_allocator.hpp>
#include <memforge/align.hpp>

#include <cstring>

namespace memforge {

namespace batch_detail {

std::size_t stride_required(std::uint32_t count, std::uint32_t stride) noexcept {
    return static_cast<std::size_t>(count) * stride;
}

std::uint32_t header_bytes_per_batch() noexcept {
    return 32;
}

std::uint32_t trailer_bytes_per_batch() noexcept {
    return 16;
}

bool batch_layout_in_bounds(std::uint32_t count, std::uint32_t stride,
                            std::uint32_t slab_bytes) noexcept {
    return stride_required(count, stride) <= slab_bytes;
}

}  // namespace batch_detail

std::uint32_t BatchAllocator::currentSlabBytes() const noexcept {
    return slab_bytes_;
}

std::uint32_t BatchAllocator::headerBytesPerBatch() noexcept {
    return batch_detail::header_bytes_per_batch();
}

std::uint32_t BatchAllocator::trailerBytesPerBatch() noexcept {
    return batch_detail::trailer_bytes_per_batch();
}

}  // namespace memforge
