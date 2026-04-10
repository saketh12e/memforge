// MemForge - alignment utilities.
//
// Provides alignment helpers used by the arena chain and the slab cache.
// These functions are header-only for inlining at hot call sites.

#ifndef MEMFORGE_ALIGN_HPP
#define MEMFORGE_ALIGN_HPP

#include <cstddef>
#include <cstdint>

namespace memforge {

constexpr std::size_t kPageSize = 4096;
constexpr std::size_t kCacheLine = 64;

inline constexpr std::size_t align_up(std::size_t value, std::size_t align) noexcept {
    if (align <= 1) {
        return value;
    }
    std::size_t rem = value & (align - 1);
    return rem == 0 ? value : value + (align - rem);
}

inline constexpr std::size_t align_down(std::size_t value, std::size_t align) noexcept {
    if (align <= 1) {
        return value;
    }
    return value & ~(align - 1);
}

inline constexpr bool is_power_of_two(std::size_t value) noexcept {
    return value != 0 && (value & (value - 1)) == 0;
}

inline constexpr std::size_t next_power_of_two(std::size_t value) noexcept {
    if (value <= 1) {
        return 1;
    }
    std::size_t r = 1;
    while (r < value) {
        r <<= 1;
    }
    return r;
}

inline constexpr std::size_t log2_floor(std::size_t value) noexcept {
    std::size_t r = 0;
    while (value > 1) {
        value >>= 1;
        ++r;
    }
    return r;
}

}  // namespace memforge

#endif  // MEMFORGE_ALIGN_HPP
