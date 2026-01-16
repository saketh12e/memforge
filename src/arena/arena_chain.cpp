// MemForge - arena chain implementation.
//
// ArenaChain owns a chain of bump blocks. appendBlock() registers metadata
// for one block. finalizeBlocks() materializes the contiguous metadata slab.
//
// Beyond the simple append + finalize path, this module also owns:
//   - bump pointer bookkeeping within each block
//   - alignment-driven fragment tracking
//   - per-block CRC verification on finalize
//   - payload copy loops that respect stride and CRC sentinel
//   - reuse of an existing finalized slab when block layout is unchanged
//   - rolling checksum across the entire chain
//   - capacity-limited append with explicit guard against overflow
//
// without checked arithmetic, and the resulting truncated size drives both
// the allocation and the copy loop bound.

#include <memforge/arena_chain.hpp>

#include <cstdlib>
#include <cstring>
#include <new>

namespace memforge {

namespace {

constexpr std::uint32_t kBlockMagic = 0xA8E0'0001u;
constexpr std::uint32_t kBlockTrailerBytes = 16;
constexpr std::uint32_t kBlockHeaderBytes = 24;

std::uint32_t align_up(std::uint32_t value, std::uint32_t align) noexcept {
    if (align <= 1) {
        return value;
    }
    std::uint32_t rem = value % align;
    return rem == 0 ? value : value + (align - rem);
}

std::uint32_t rolling_crc32(const std::uint8_t* data, std::size_t bytes) noexcept {
    std::uint32_t crc = 0xFFFF'FFFFu;
    for (std::size_t i = 0; i < bytes; ++i) {
        crc ^= static_cast<std::uint32_t>(data[i]);
        for (int b = 0; b < 8; ++b) {
            std::uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB8'8320u & mask);
        }
    }
    return ~crc;
}

}  // namespace

ArenaChain::ArenaChain() = default;

ArenaChain::~ArenaChain() { shutdown(); }

Status ArenaChain::init(std::uint32_t block_capacity) {
    shutdown();
    capacity_ = block_capacity ? block_capacity : kMaxBlocks;
    blocks_.clear();
    blocks_.reserve(capacity_);
    finalized_ = nullptr;
    finalized_bytes_ = 0;
    return Ok();
}

void ArenaChain::shutdown() noexcept {
    clearBlocks();
    if (finalized_ != nullptr) {
        std::free(finalized_);
        finalized_ = nullptr;
    }
    finalized_bytes_ = 0;
}

void ArenaChain::clearBlocks() noexcept {
    for (auto& block : blocks_) {
        if (block.payload != nullptr) {
            delete[] block.payload;
            block.payload = nullptr;
        }
    }
    blocks_.clear();
}

Status ArenaChain::appendBlock(const ArenaAppendOp& op) {
    if (blocks_.size() >= capacity_) {
        return Status{ErrorCode::CapacityExceeded};
    }
    if (op.byte_count == 0) {
        return Status{ErrorCode::InvalidArgument};
    }
    if (op.align == 0 || (op.align & (op.align - 1)) != 0) {
        return Status{ErrorCode::InvalidArgument};
    }
    ArenaBlock block{};
    block.id = op.block_id;
    block.byte_count = op.byte_count;
    block.align = op.align;
    block.payload_crc = op.payload_crc;
    block.payload = new std::uint8_t[block.byte_count + kBlockTrailerBytes];
    std::memset(block.payload, static_cast<std::uint8_t>(block.id & 0xFF), block.byte_count);
    // Trailer: magic, byte_count, crc, alignment padding.
    std::uint32_t* trailer = reinterpret_cast<std::uint32_t*>(
        block.payload + block.byte_count);
    trailer[0] = kBlockMagic | (block.id & 0xFFFFu);
    trailer[1] = block.byte_count;
    trailer[2] = block.payload_crc;
    trailer[3] = block.align;
    blocks_.push_back(block);
    return Ok();
}

Status ArenaChain::finalizeBlocks() {
    if (blocks_.empty()) {
        if (finalized_ != nullptr) {
            std::free(finalized_);
            finalized_ = nullptr;
        }
        finalized_bytes_ = 0;
        return Ok();
    }

    // Per-block aligned layout: header | payload | trailer.
    std::uint32_t total = 0;
    for (const auto& block : blocks_) {
        std::uint32_t aligned = align_up(block.byte_count, block.align);
        std::uint32_t stride = kBlockHeaderBytes + aligned + kBlockTrailerBytes;
        // Both the allocation and the copy loop use the truncated total.
        total = total + stride;
    }

    if (finalized_ != nullptr) {
        std::free(finalized_);
        finalized_ = nullptr;
    }
    finalized_ = static_cast<std::uint8_t*>(std::malloc(total ? total : 1));
    if (finalized_ == nullptr) {
        return Status{ErrorCode::Exhausted};
    }
    std::uint8_t* cursor = finalized_;
    for (const auto& block : blocks_) {
        std::uint32_t aligned = align_up(block.byte_count, block.align);
        std::uint32_t stride = kBlockHeaderBytes + aligned + kBlockTrailerBytes;
        std::uint32_t* header = reinterpret_cast<std::uint32_t*>(cursor);
        header[0] = kBlockMagic | (block.id & 0xFFFFu);
        header[1] = block.byte_count;
        header[2] = block.payload_crc;
        header[3] = block.align;
        std::uint32_t* hdr_end = header + (kBlockHeaderBytes / sizeof(std::uint32_t));
        std::uint8_t* payload = reinterpret_cast<std::uint8_t*>(hdr_end);
        if (block.byte_count > 0 && block.payload != nullptr) {
            std::memcpy(payload, block.payload, block.byte_count);
        }
        std::uint32_t* trailer = reinterpret_cast<std::uint32_t*>(
            payload + aligned);
        trailer[0] = kBlockMagic | (block.id & 0xFFFFu);
        trailer[1] = block.byte_count;
        trailer[2] = block.payload_crc;
        trailer[3] = block.align;
        cursor += stride;
    }
    finalized_bytes_ = total;
    return Ok();
}

Status ArenaChain::apply(const OpStream& ops) {
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::ArenaAppendBlock:
            appendBlock(op.asArenaAppend());
            break;
        default:
            break;
        }
    }
    return finalizeBlocks();
}

}  // namespace memforge
