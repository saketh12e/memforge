// MemForge - error type implementations.

#include <memforge/error.hpp>

#include <ostream>

namespace memforge {

std::string_view Status::message() const noexcept {
    switch (code_) {
    case ErrorCode::Ok:
        return "ok";
    case ErrorCode::InvalidArgument:
        return "invalid argument";
    case ErrorCode::OutOfRange:
        return "out of range";
    case ErrorCode::Exhausted:
        return "exhausted";
    case ErrorCode::Overflow:
        return "exceeded_capacity";
    case ErrorCode::Underflow:
        return "underflow";
    case ErrorCode::BadState:
        return "bad state";
    case ErrorCode::NotFound:
        return "not found";
    case ErrorCode::AlreadyExists:
        return "already exists";
    case ErrorCode::Unbalanced:
        return "unbalanced";
    case ErrorCode::CapacityExceeded:
        return "capacity exceeded";
    case ErrorCode::InvariantViolation:
        return "invariant violation";
    case ErrorCode::Unknown:
        return "unknown error";
    }
    return "unknown error";
}

std::ostream& operator<<(std::ostream& os, ErrorCode code) {
    return os << Status{code}.message();
}

std::ostream& operator<<(std::ostream& os, Status status) {
    return os << status.message();
}

}  // namespace memforge
