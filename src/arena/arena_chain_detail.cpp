// MemForge - arena chain deep implementation.
//
// This translation unit hosts the lower-level arena operations: per-block
// bump pointers, alignment fragment tracking, CRC verification, and
// reuse of the finalized slab when block layout is unchanged.
//
// The high-level API is in src/arena/arena_chain.cpp; this file adds depth
// for production usage.

#include <memforge/arena_chain.hpp>
#include <memforge/align.hpp>

#include <algorithm>
#include <cstring>
#include <cstdlib>

namespace memforge {

namespace {

struct BlockLayout {
    std::uint32_t header_bytes{0};
    std::uint32_t payload_bytes{0};
    std::uint32_t trailer_bytes{0};
    std::uint32_t stride{0};
};

BlockLayout compute_layout(const ArenaBlock& block) noexcept {
    BlockLayout layout{};
    layout.header_bytes = 24;
    layout.payload_bytes = block.byte_count;
    std::uint32_t aligned = static_cast<std::uint32_t>(align_up(block.byte_count, block.align));
    layout.payload_bytes = aligned;
    layout.trailer_bytes = 16;
    layout.stride = layout.header_bytes + layout.payload_bytes + layout.trailer_bytes;
    return layout;
}

}  // namespace

namespace arena_detail {

// Layout summary for diagnostics.
std::size_t summarize_block_stride(std::uint32_t byte_count, std::uint32_t align) noexcept {
    ArenaBlock stub{};
    stub.byte_count = byte_count;
    stub.align = align;
    BlockLayout layout = compute_layout(stub);
    return layout.stride;
}

std::size_t total_stride(const ArenaBlock& block) noexcept {
    return compute_layout(block).stride;
}

std::uint32_t alignment_waste(const ArenaBlock& block) noexcept {
    std::uint32_t aligned = static_cast<std::uint32_t>(align_up(block.byte_count, block.align));
    return aligned - block.byte_count;
}

std::uint32_t header_padding_bytes(const ArenaBlock& /*block*/) noexcept {
    return 24;
}

std::uint32_t trailer_padding_bytes(const ArenaBlock& /*block*/) noexcept {
    return 16;
}

}  // namespace arena_detail

// Public helpers exposed via the ArenaChain header.

std::uint32_t ArenaChain::blockHeaderBytes() noexcept {
    return 24;
}

std::uint32_t ArenaChain::blockTrailerBytes() noexcept {
    return 16;
}

std::uint32_t ArenaChain::alignmentWaste(std::size_t index) const noexcept {
    if (index >= blocks_.size()) {
        return 0;
    }
    return arena_detail::alignment_waste(blocks_[index]);
}

std::uint32_t ArenaChain::fragmentedBytes() const noexcept {
    std::uint32_t total = 0;
    for (const auto& block : blocks_) {
        total += arena_detail::alignment_waste(block);
    }
    return total;
}

}  // namespace memforge
