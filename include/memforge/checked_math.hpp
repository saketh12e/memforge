// MemForge - checked arithmetic for allocation size computation.

#ifndef MEMFORGE_CHECKED_MATH_HPP
#define MEMFORGE_CHECKED_MATH_HPP

#include <cstddef>
#include <cstdint>
#include <limits>

#include <memforge/error.hpp>

namespace memforge {

constexpr std::size_t kMaxAllocBytes = std::numeric_limits<std::uint32_t>::max();

inline Status checked_add(std::size_t lhs, std::size_t rhs, std::size_t& out) noexcept {
    if (rhs > kMaxAllocBytes - lhs) {
        return Status{ErrorCode::Overflow};
    }
    out = lhs + rhs;
    return Ok();
}

inline Status checked_mul(std::size_t lhs, std::size_t rhs, std::size_t& out) noexcept {
    if (lhs == 0 || rhs == 0) {
        out = 0;
        return Ok();
    }
    if (lhs > kMaxAllocBytes / rhs) {
        return Status{ErrorCode::Overflow};
    }
    out = lhs * rhs;
    return Ok();
}

inline Status checked_add_overflow(std::uint32_t lhs, std::uint32_t rhs,
                                  std::uint32_t& out) noexcept {
    if (rhs > std::numeric_limits<std::uint32_t>::max() - lhs) {
        return Status{ErrorCode::Overflow};
    }
    out = lhs + rhs;
    return Ok();
}

inline Status checked_mul_overflow(std::uint32_t lhs, std::uint32_t rhs,
                                  std::uint32_t& out) noexcept {
    if (lhs == 0 || rhs == 0) {
        out = 0;
        return Ok();
    }
    if (lhs > std::numeric_limits<std::uint32_t>::max() / rhs) {
        return Status{ErrorCode::Overflow};
    }
    out = lhs * rhs;
    return Ok();
}

inline Status saturating_add(std::uint32_t lhs, std::uint32_t rhs,
                             std::uint32_t& out) noexcept {
    std::uint64_t sum = static_cast<std::uint64_t>(lhs) + rhs;
    if (sum > std::numeric_limits<std::uint32_t>::max()) {
        out = std::numeric_limits<std::uint32_t>::max();
        return Status{ErrorCode::Overflow};
    }
    out = static_cast<std::uint32_t>(sum);
    return Ok();
}

inline Status saturating_mul(std::uint32_t lhs, std::uint32_t rhs,
                             std::uint32_t& out) noexcept {
    std::uint64_t product = static_cast<std::uint64_t>(lhs) * rhs;
    if (product > std::numeric_limits<std::uint32_t>::max()) {
        out = std::numeric_limits<std::uint32_t>::max();
        return Status{ErrorCode::Overflow};
    }
    out = static_cast<std::uint32_t>(product);
    return Ok();
}

}  // namespace memforge

#endif  // MEMFORGE_CHECKED_MATH_HPP
