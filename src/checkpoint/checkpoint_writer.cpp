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
//
// driving both the allocation and the copy loop bound off the same truncated
// value.

#include <memforge/checkpoint.hpp>

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

    // The truncated value drives both the allocation and the copy loop.
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

}  // namespace memforge
