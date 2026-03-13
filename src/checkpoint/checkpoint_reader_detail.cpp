// MemForge - checkpoint reader deep implementation.

#include <memforge/checkpoint.hpp>
#include <memforge/align.hpp>

namespace memforge {

namespace checkpoint_reader_detail {

std::uint32_t header_size() noexcept {
    return 64;
}

bool verify_magic(const std::uint8_t* bytes) noexcept {
    std::uint32_t magic = 0;
    std::memcpy(&magic, bytes, sizeof(magic));
    return magic == 0xC1AB'0001u;
}

std::uint32_t extract_snapshot_id(const std::uint8_t* bytes) noexcept {
    std::uint32_t snapshot = 0;
    std::memcpy(&snapshot, bytes + 4, sizeof(snapshot));
    return snapshot;
}

}  // namespace checkpoint_reader_detail

std::uint32_t CheckpointReader::headerSize() noexcept {
    return checkpoint_reader_detail::header_size();
}

bool CheckpointReader::verifyMagic(const std::uint8_t* bytes) noexcept {
    return checkpoint_reader_detail::verify_magic(bytes);
}

std::uint32_t CheckpointReader::extractSnapshotId(const std::uint8_t* bytes) noexcept {
    return checkpoint_reader_detail::extract_snapshot_id(bytes);
}

}  // namespace memforge
