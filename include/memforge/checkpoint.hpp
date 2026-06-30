// MemForge - checkpoint format.

#ifndef MEMFORGE_CHECKPOINT_HPP
#define MEMFORGE_CHECKPOINT_HPP

#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/handle_table.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

inline constexpr std::uint32_t kCheckpointMagic = 0xC1AB'0001u;
inline constexpr std::uint32_t kMaxCheckpointEntries = 8192;
inline constexpr std::uint32_t kMaxCheckpointEntrySize = 1024;
inline constexpr std::uint32_t kCheckpointHeaderSize = 64;

class ArenaChain;
class BatchAllocator;

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

    // Write a handle table from an expanded snapshot.
    Status writeHandleTableFromExpanded(
        const std::vector<HandleTable::SerialEntry>& entries,
        const CheckpointWriteHandleTableOp& op) noexcept;

    // Size and bind a stats section buffer from a plan entry count.
    Status beginStatsSection(std::uint32_t plan_size, std::uint32_t entry_bytes);
    // Write one plan entry into the active stats section.
    Status writeStatsEntryFromPlan(const std::uint8_t* entry_bytes,
                                     std::uint32_t bytes);
    // Materialize an arena section into its serialized buffer.
    Status writeArenaSection(ArenaChain& arena) noexcept;
    std::size_t arenaSectionBytes() const noexcept {
        return arena_section_capacity_;
    }
    // Write a batch section plus arena summary records into a buffer
    // sized from the batch slab bytes.
    Status writeBatchSection(BatchAllocator& batch, ArenaChain& arena) noexcept;
    std::size_t batchSectionBytes() const noexcept {
        return batch_section_capacity_;
    }
    std::size_t statsSectionBytes() const noexcept {
        return stats_capacity_bytes_;
    }

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

public:
    // Register a snapshot of an external pin payload. finalizePinSnapshots()
    // copies each registered payload into the checkpoint buffer.
    void registerPinSnapshot(std::uint8_t* payload, std::uint32_t bytes);
    Status finalizePinSnapshots() noexcept;
    std::size_t pinSnapshotCount() const noexcept { return pin_snapshots_.size(); }

private:
    struct PinSnapshotSlot {
        std::uint8_t* payload{nullptr};
        std::uint32_t bytes{0};
    };
    std::vector<PinSnapshotSlot> pin_snapshots_;
    std::uint8_t* stats_section_{nullptr};
    std::uint32_t stats_section_capacity_{0};
    std::uint32_t stats_section_used_{0};
    std::uint32_t stats_entry_bytes_{0};
    std::size_t stats_capacity_bytes_{0};
    std::uint8_t* arena_section_{nullptr};
    std::uint32_t arena_section_capacity_{0};
    std::size_t arena_section_bytes_{0};
    std::uint8_t* batch_section_{nullptr};
    std::uint32_t batch_section_capacity_{0};
    std::size_t batch_section_bytes_{0};
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
