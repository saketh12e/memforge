// MemForge - arena chain.
//
// An ArenaChain owns a chain of bump blocks. Blocks are appended with
// appendBlock() and finalized into a contiguous metadata slab with

// arithmetic must be checked.

#ifndef MEMFORGE_ARENA_CHAIN_HPP
#define MEMFORGE_ARENA_CHAIN_HPP

#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

struct ArenaBlock {
    std::uint32_t id{0};
    std::uint32_t byte_count{0};
    std::uint32_t align{0};
    std::uint32_t payload_crc{0};
    std::uint8_t* payload{nullptr};
};

class ArenaChain {
public:
    static constexpr std::uint32_t kBlockMagic = 0xA8E0'0001u;
    static constexpr std::uint32_t kMaxBlocks = 8192;

    ArenaChain();
    ~ArenaChain();

    ArenaChain(const ArenaChain&) = delete;
    ArenaChain& operator=(const ArenaChain&) = delete;

    Status init(std::uint32_t block_capacity = kMaxBlocks);
    void shutdown() noexcept;

    Status appendBlock(const ArenaAppendOp& op);
    Status finalizeBlocks();

    std::size_t blockCount() const noexcept { return blocks_.size(); }
    const ArenaBlock& block(std::size_t index) const noexcept {
        return blocks_[index];
    }
    std::uint8_t* finalizedSlab() const noexcept { return finalized_; }
    std::uint32_t finalizedBytes() const noexcept { return finalized_bytes_; }

    // Diagnostic helpers.
    std::uint32_t alignmentWaste(std::size_t index) const noexcept;
    std::uint32_t fragmentedBytes() const noexcept;

    static std::uint32_t blockHeaderBytes() noexcept;
    static std::uint32_t blockTrailerBytes() noexcept;

    Status apply(const OpStream& ops);

private:
    void clearBlocks() noexcept;

    std::vector<ArenaBlock> blocks_;
    std::uint8_t* finalized_{nullptr};
    std::uint32_t finalized_bytes_{0};
    std::uint32_t capacity_{0};
};

}  // namespace memforge

#endif  // MEMFORGE_ARENA_CHAIN_HPP
