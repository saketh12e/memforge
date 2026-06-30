// MemForge - checked arithmetic helpers.
//
// The header holds the inline versions; this translation unit keeps the
// checked-math API stable for CMake consumers and exposes additional

#include <memforge/checked_math.hpp>

#include <cstdint>
#include <limits>

namespace memforge {

Status checked_add_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& out) noexcept {
    if (rhs > std::numeric_limits<std::uint64_t>::max() - lhs) {
        return Status{ErrorCode::Overflow};
    }
    out = lhs + rhs;
    return Ok();
}

Status checked_mul_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& out) noexcept {
    if (lhs == 0 || rhs == 0) {
        out = 0;
        return Ok();
    }
    if (lhs > std::numeric_limits<std::uint64_t>::max() / rhs) {
        return Status{ErrorCode::Overflow};
    }
    out = lhs * rhs;
    return Ok();
}

}  // namespace memforge
