// MemForge - checkpoint format.

#ifndef MEMFORGE_CHECKPOINT_HPP
#define MEMFORGE_CHECKPOINT_HPP

#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

inline constexpr std::uint32_t kCheckpointMagic = 0xC1AB'0001u;
inline constexpr std::uint32_t kMaxCheckpointEntries = 8192;
inline constexpr std::uint32_t kMaxCheckpointEntrySize = 1024;
inline constexpr std::uint32_t kCheckpointHeaderSize = 64;

class CheckpointWriter {
public:
    CheckpointWriter();
    ~CheckpointWriter();

    CheckpointWriter(const CheckpointWriter&) = delete;
    CheckpointWriter& operator=(const CheckpointWriter&) = delete;

    Status init();
    void shutdown() noexcept;

    Status begin(const CheckpointBeginOp& op);
    Status writeHandleTable(const CheckpointWriteHandleTableOp& op);

    std::size_t byteSize() const noexcept { return buffer_.size(); }
    const std::vector<std::uint8_t>& buffer() const noexcept { return buffer_; }

    static std::uint32_t headerSize() noexcept;
    static std::uint32_t sectionOverhead() noexcept;
    static std::uint32_t crc32(const std::uint8_t* data, std::size_t bytes) noexcept;

    Status apply(const OpStream& ops);

private:
    std::vector<std::uint8_t> buffer_;
    std::uint32_t entry_count_{0};
    std::uint32_t entry_size_{0};
    std::uint32_t snapshot_id_{0};
    std::uint8_t* table_slab_{nullptr};
    std::uint32_t table_slab_bytes_{0};
    bool begun_{false};
};

class CheckpointReader {
public:
    CheckpointReader();
    ~CheckpointReader();

    CheckpointReader(const CheckpointReader&) = delete;
    CheckpointReader& operator=(const CheckpointReader&) = delete;

    Status init();
    void shutdown() noexcept;

    Status read(ConstByteSpan bytes, std::vector<std::uint8_t>& out);

    static std::uint32_t headerSize() noexcept;
    static bool verifyMagic(const std::uint8_t* bytes) noexcept;
    static std::uint32_t extractSnapshotId(const std::uint8_t* bytes) noexcept;

private:
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_CHECKPOINT_HPP
