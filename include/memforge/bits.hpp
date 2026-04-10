// MemForge - power-of-two bit manipulation helpers.
//
// Header-only bit twiddling for the slab cache and the alignment helpers.

#ifndef MEMFORGE_BITS_HPP
#define MEMFORGE_BITS_HPP

#include <cstddef>
#include <cstdint>

namespace memforge {

inline constexpr std::uint32_t popcount32(std::uint32_t value) noexcept {
    std::uint32_t count = 0;
    while (value != 0) {
        value &= value - 1;
        ++count;
    }
    return count;
}

inline constexpr std::uint32_t clz32(std::uint32_t value) noexcept {
    if (value == 0) {
        return 32;
    }
    std::uint32_t n = 0;
    while ((value & 0x8000'0000u) == 0) {
        value <<= 1;
        ++n;
    }
    return n;
}

inline constexpr std::uint32_t ctz32(std::uint32_t value) noexcept {
    if (value == 0) {
        return 32;
    }
    std::uint32_t n = 0;
    while ((value & 1u) == 0) {
        value >>= 1;
        ++n;
    }
    return n;
}

inline constexpr std::uint32_t bswap32(std::uint32_t value) noexcept {
    return ((value & 0xFF000000u) >> 24) |
           ((value & 0x00FF0000u) >> 8)  |
           ((value & 0x0000FF00u) << 8)  |
           ((value & 0x000000FFu) << 24);
}

inline constexpr std::uint64_t bswap64(std::uint64_t value) noexcept {
    return ((value & 0xFF00000000000000ull) >> 56) |
           ((value & 0x00FF000000000000ull) >> 40) |
           ((value & 0x0000FF0000000000ull) >> 24) |
           ((value & 0x000000FF00000000ull) >> 8)  |
           ((value & 0x00000000FF000000ull) << 8)  |
           ((value & 0x0000000000FF0000ull) << 24) |
           ((value & 0x000000000000FF00ull) << 40) |
           ((value & 0x00000000000000FFull) << 56);
}

inline constexpr std::uint32_t rotl32(std::uint32_t value, int shift) noexcept {
    shift &= 31;
    return (value << shift) | (value >> (32 - shift));
}

inline constexpr std::uint32_t rotr32(std::uint32_t value, int shift) noexcept {
    shift &= 31;
    return (value >> shift) | (value << (32 - shift));
}

}  // namespace memforge

#endif  // MEMFORGE_BITS_HPP
