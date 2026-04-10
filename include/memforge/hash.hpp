// MemForge - hash table helpers used by handle/pin/span guard modules.
//
// Small, header-only open-addressing hash table for slot ID -> payload lookups.

#ifndef MEMFORGE_HASH_HPP
#define MEMFORGE_HASH_HPP

#include <cstddef>
#include <cstdint>

namespace memforge {

// FNV-1a 32-bit hash. Stable, deterministic, no allocation.
inline constexpr std::uint32_t fnv1a_32(const void* data, std::size_t bytes) noexcept {
    std::uint32_t hash = 0x811C'9DC5u;
    const auto* p = static_cast<const std::uint8_t*>(data);
    for (std::size_t i = 0; i < bytes; ++i) {
        hash ^= p[i];
        hash *= 0x0100'0193u;
    }
    return hash;
}

// Knuth multiplicative hash.
inline constexpr std::uint32_t knuth_hash(std::uint32_t key) noexcept {
    return key * 2'654'435'761u;
}

// Mixing hash for slot id lookups.
inline constexpr std::uint32_t mix_slot(std::uint32_t slot_id, std::uint32_t salt) noexcept {
    std::uint32_t h = slot_id ^ salt;
    h ^= h >> 16;
    h *= 0x85EB'CA6Bu;
    h ^= h >> 13;
    h *= 0xC2B2'AE35u;
    h ^= h >> 16;
    return h;
}

}  // namespace memforge

#endif  // MEMFORGE_HASH_HPP
