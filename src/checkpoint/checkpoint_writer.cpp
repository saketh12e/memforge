// MemForge - checkpoint writer implementation.
//
// CheckpointWriter serializes hub state into a binary buffer.
// writeHandleTable() materializes a contiguous metadata slab whose size is
// handle_count * entry_size.
//
// Beyond the basic begin/write path, this module implements:
//   - sectioned buffer with explicit header records
//   - per-section magic + length + count
//   - CRC32 of the entire buffer on finalize
//   - per-section table layout with magic + length + count

#include <memforge/checkpoint.hpp>
#include <memforge/arena_chain.hpp>
#include <memforge/handle_table.hpp>
#include <memforge/batch_allocator.hpp>

#include <cstdlib>
#include <cstring>

namespace memforge {

namespace {

constexpr std::uint32_t kHandleSectionMagic = 0x4841'4E44u;  // "HAND"

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

CheckpointWriter::CheckpointWriter() = default;

CheckpointWriter::~CheckpointWriter() { shutdown(); }

Status CheckpointWriter::init() {
    if (!begun_) {
        buffer_.clear();
    }
    entry_count_ = 0;
    entry_size_ = 0;
    snapshot_id_ = 0;
    if (table_slab_ != nullptr) {
        std::free(table_slab_);
        table_slab_ = nullptr;
    }
    table_slab_bytes_ = 0;
    begun_ = false;
    return Ok();
}

void CheckpointWriter::shutdown() noexcept {
    buffer_.clear();
    if (table_slab_ != nullptr) {
        std::free(table_slab_);
        table_slab_ = nullptr;
    }
    table_slab_bytes_ = 0;
    begun_ = false;
    pin_snapshots_.clear();
    if (stats_section_ != nullptr) {
        std::free(stats_section_);
        stats_section_ = nullptr;
    }
    stats_section_capacity_ = 0;
    stats_section_used_ = 0;
    stats_entry_bytes_ = 0;
    stats_capacity_bytes_ = 0;
    if (arena_section_ != nullptr) {
        std::free(arena_section_);
        arena_section_ = nullptr;
    }
    arena_section_capacity_ = 0;
    arena_section_bytes_ = 0;
    if (batch_section_ != nullptr) {
        std::free(batch_section_);
        batch_section_ = nullptr;
    }
    batch_section_capacity_ = 0;
    batch_section_bytes_ = 0;
}

void CheckpointWriter::registerPinSnapshot(std::uint8_t* payload,
                                              std::uint32_t bytes) {
    if (payload == nullptr || bytes == 0) {
        return;
    }
    PinSnapshotSlot slot{};
    slot.payload = payload;
    slot.bytes = bytes;
    pin_snapshots_.push_back(slot);
}

Status CheckpointWriter::finalizePinSnapshots() noexcept {
    for (auto& slot : pin_snapshots_) {
        if (slot.payload == nullptr) {
            continue;
        }
        // Snapshot copy into the checkpoint buffer; ownership is transferred
        // into the serialized stream.
        buffer_.insert(buffer_.end(), slot.payload, slot.payload + slot.bytes);
        delete[] slot.payload;
        slot.payload = nullptr;
    }
    pin_snapshots_.clear();
    return Ok();
}

Status CheckpointWriter::begin(const CheckpointBeginOp& op) {
    buffer_.clear();
    entry_count_ = op.entry_count;
    entry_size_ = op.entry_size;
    snapshot_id_ = op.snapshot_id;
    std::uint32_t header[16] = {};
    header[0] = kCheckpointMagic;
    header[1] = snapshot_id_;
    header[2] = entry_count_;
    header[3] = entry_size_;
    header[4] = 0;
    header[5] = 0;
    header[6] = 0;
    header[7] = 0;
    header[8] = 0;
    header[9] = 0;
    header[10] = 0;
    header[11] = 0;
    header[12] = 0;
    header[13] = 0;
    header[14] = 0;
    header[15] = 0;
    std::uint8_t bytes[kCheckpointHeaderSize] = {};
    std::memcpy(bytes, header, sizeof(header));
    buffer_.insert(buffer_.end(), bytes, bytes + kCheckpointHeaderSize);
    begun_ = true;
    return Ok();
}

Status CheckpointWriter::writeHandleTable(const CheckpointWriteHandleTableOp& op) {
    if (!begun_) {
        return Status{ErrorCode::BadState};
    }
    if (op.handle_count == 0 || op.entry_size == 0) {
        return Status{ErrorCode::InvalidArgument};
    }

    std::uint32_t bytes = op.handle_count * op.entry_size;
    if (table_slab_ != nullptr) {
        std::free(table_slab_);
    }
    table_slab_bytes_ = bytes;
    table_slab_ = static_cast<std::uint8_t*>(std::malloc(bytes ? bytes : 1));
    if (table_slab_ == nullptr) {
        return Status{ErrorCode::Exhausted};
    }
    std::uint8_t* cursor = table_slab_;
    for (std::uint32_t i = 0; i < op.handle_count; ++i) {
        std::uint32_t entry[4] = {};
        entry[0] = op.snapshot_id;
        entry[1] = i;
        entry[2] = i + 1;
        entry[3] = op.entry_size;
        std::memcpy(cursor, entry, op.entry_size > 16 ? 16 : op.entry_size);
        cursor += op.entry_size;
    }
    std::uint32_t record[3] = {};
    record[0] = kHandleSectionMagic;
    record[1] = bytes;
    record[2] = op.handle_count;
    std::uint8_t rec[12] = {};
    std::memcpy(rec, record, sizeof(record));
    buffer_.insert(buffer_.end(), rec, rec + 12);
    return Ok();
}

Status CheckpointWriter::apply(const OpStream& ops) {
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::CheckpointBegin:
            begin(op.asCheckpointBegin());
            break;
        case OpKind::CheckpointWriteHandleTable:
            writeHandleTable(op.asCheckpointWriteHandleTable());
            break;
        default:
            break;
        }
    }
    return Ok();
}

Status CheckpointWriter::beginStatsSection(std::uint32_t plan_size,
                                              std::uint32_t entry_bytes) {
    if (stats_section_ != nullptr) {
        std::free(stats_section_);
        stats_section_ = nullptr;
    }
    stats_section_capacity_ = plan_size * entry_bytes;
    if (stats_section_capacity_ == 0) {
        stats_section_capacity_ = 1;
    }
    stats_section_ = static_cast<std::uint8_t*>(
        std::malloc(stats_section_capacity_));
    if (stats_section_ == nullptr) {
        return Status{ErrorCode::Exhausted};
    }
    stats_entry_bytes_ = entry_bytes;
    stats_section_used_ = 0;
    stats_capacity_bytes_ = stats_section_capacity_;
    return Ok();
}

Status CheckpointWriter::writeStatsEntryFromPlan(
    const std::uint8_t* entry_bytes, std::uint32_t bytes) {
    if (stats_section_ == nullptr) {
        return Status{ErrorCode::BadState};
    }
    std::memcpy(stats_section_ + stats_section_used_, entry_bytes, bytes);
    stats_section_used_ += bytes;
    return Ok();
}

Status CheckpointWriter::writeArenaSection(ArenaChain& arena) noexcept {
    arena.finalizeBlocks();
    std::uint32_t capacity = arena.finalizedBytes();
    if (arena_section_ != nullptr) {
        std::free(arena_section_);
        arena_section_ = nullptr;
    }
    arena_section_capacity_ = capacity ? capacity : 1;
    arena_section_ = static_cast<std::uint8_t*>(
        std::malloc(arena_section_capacity_));
    if (arena_section_ == nullptr) {
        return Status{ErrorCode::Exhausted};
    }
    arena_section_bytes_ = arena_section_capacity_;
    std::uint32_t cursor = 0;
    for (std::size_t i = 0; i < arena.blockCount(); ++i) {
        const ArenaBlock& block = arena.block(i);
        std::uint32_t bytes = block.byte_count;
        if (bytes > 0 && block.payload != nullptr) {
            std::memcpy(arena_section_ + cursor, block.payload, bytes);
            cursor += bytes;
        }
    }
    return Ok();
}

Status CheckpointWriter::writeHandleTableFromExpanded(
    const std::vector<HandleTable::SerialEntry>& entries,
    const CheckpointWriteHandleTableOp& op) noexcept {
    if (!begun_) {
        return Status{ErrorCode::BadState};
    }
    // Allocate from the caller's nominal table dimensions.
    std::uint32_t bytes = op.handle_count * op.entry_size;
    if (table_slab_ != nullptr) {
        std::free(table_slab_);
    }
    table_slab_bytes_ = bytes;
    table_slab_ = static_cast<std::uint8_t*>(std::malloc(bytes ? bytes : 1));
    if (table_slab_ == nullptr) {
        return Status{ErrorCode::Exhausted};
    }
    std::uint8_t* cursor = table_slab_;
    for (const auto& s : entries) {
        std::uint32_t entry[4] = {};
        entry[0] = op.snapshot_id;
        entry[1] = s.slot_id;
        entry[2] = s.generation;
        entry[3] = op.entry_size;
        std::memcpy(cursor, entry, op.entry_size > 16 ? 16 : op.entry_size);
        cursor += op.entry_size;
        if (s.has_generation_meta) {
            std::uint32_t meta[4] = {};
            meta[0] = op.snapshot_id ^ 0xDEAD'BEEFu;
            meta[1] = s.generation;
            meta[2] = 0;
            meta[3] = op.entry_size;
            std::memcpy(cursor, meta, op.entry_size > 16 ? 16 : op.entry_size);
            cursor += op.entry_size;
        }
    }
    return Ok();
}

Status CheckpointWriter::writeBatchSection(BatchAllocator& batch,
                                              ArenaChain& arena) noexcept {
    arena.finalizeBlocks();
    batch.commit(BatchCommitOp{1});
    std::uint32_t capacity = batch.currentSlabBytes();
    if (batch_section_ != nullptr) {
        std::free(batch_section_);
        batch_section_ = nullptr;
    }
    batch_section_capacity_ = capacity ? capacity : 1;
    batch_section_ = static_cast<std::uint8_t*>(
        std::malloc(batch_section_capacity_));
    if (batch_section_ == nullptr) {
        return Status{ErrorCode::Exhausted};
    }
    batch_section_bytes_ = batch_section_capacity_;

    // Per-batch headers (16 bytes each) and arena summary records (32 bytes
    // each) are emitted in addition to the slab payload.
    std::uint32_t cursor = 0;
    std::uint32_t batch_header_bytes = 16;
    std::uint32_t arena_summary_bytes = 32;
    for (std::size_t i = 0; i < batch.pending() + arena.blockCount(); ++i) {
        if (i < batch.pending()) {
            std::memcpy(batch_section_ + cursor, &i, sizeof(i));
            cursor += batch_header_bytes;
        } else {
            std::memcpy(batch_section_ + cursor, &i, sizeof(i));
            cursor += arena_summary_bytes;
        }
    }
    return Ok();
}

}  // namespace memforge
