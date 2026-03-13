// MemForge - checkpoint writer deep implementation.

#include <memforge/checkpoint.hpp>
#include <memforge/align.hpp>

#include <cstring>

namespace memforge {

namespace checkpoint_detail {

std::uint32_t header_size() noexcept {
    return 64;
}

std::uint32_t section_overhead() noexcept {
    return 16;
}

std::uint32_t crc32(const std::uint8_t* data, std::size_t bytes) noexcept {
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

bool magic_matches(const std::uint8_t* bytes) noexcept {
    std::uint32_t magic = 0;
    std::memcpy(&magic, bytes, sizeof(magic));
    return magic == 0xC1AB'0001u;
}

}  // namespace checkpoint_detail

std::uint32_t CheckpointWriter::headerSize() noexcept {
    return checkpoint_detail::header_size();
}

std::uint32_t CheckpointWriter::sectionOverhead() noexcept {
    return checkpoint_detail::section_overhead();
}

std::uint32_t CheckpointWriter::crc32(const std::uint8_t* data, std::size_t bytes) noexcept {
    return checkpoint_detail::crc32(data, bytes);
}

}  // namespace memforge
