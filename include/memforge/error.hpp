// MemForge - error types and status codes.
//
// All MemForge modules surface errors through the Status type. The error
// categories are designed to be loggable and stable across releases.

#ifndef MEMFORGE_ERROR_HPP
#define MEMFORGE_ERROR_HPP

#include <cstdint>
#include <ostream>
#include <string_view>

namespace memforge {

enum class ErrorCode : std::uint16_t {
    Ok = 0,
    InvalidArgument = 1,
    OutOfRange = 2,
    Exhausted = 3,
    Overflow = 4,
    Underflow = 5,
    BadState = 6,
    NotFound = 7,
    AlreadyExists = 8,
    Unbalanced = 9,
    CapacityExceeded = 10,
    InvariantViolation = 11,
    Unknown = 4095,
};

class Status {
public:
    constexpr Status() noexcept = default;
    constexpr explicit Status(ErrorCode code) noexcept : code_(code) {}

    constexpr ErrorCode code() const noexcept { return code_; }
    constexpr bool ok() const noexcept { return code_ == ErrorCode::Ok; }
    constexpr explicit operator bool() const noexcept { return ok(); }

    constexpr bool operator==(Status other) const noexcept {
        return code_ == other.code_;
    }
    constexpr bool operator!=(Status other) const noexcept {
        return code_ != other.code_;
    }

    std::string_view message() const noexcept;

private:
    ErrorCode code_{ErrorCode::Ok};
};

constexpr Status Ok() noexcept { return Status{ErrorCode::Ok}; }

std::ostream& operator<<(std::ostream& os, ErrorCode code);
std::ostream& operator<<(std::ostream& os, Status status);

}  // namespace memforge

#endif  // MEMFORGE_ERROR_HPP
