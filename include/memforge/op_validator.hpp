// MemForge - op stream validator.
//
// Walks an op stream and validates that each op has well-formed operands for
// its kind. Used by the CLI to gate --inspect and --stats outputs, and by the
// fuzz harnesses as a sanity-check layer.

#ifndef MEMFORGE_OP_VALIDATOR_HPP
#define MEMFORGE_OP_VALIDATOR_HPP

#include <cstdint>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

struct ValidationReport {
    std::uint32_t total{0};
    std::uint32_t valid{0};
    std::uint32_t invalid{0};
    std::uint32_t unsupported{0};
    Status first_error{};
    std::size_t first_error_index{0};
};

class OpStreamValidator {
public:
    OpStreamValidator() = default;
    ~OpStreamValidator() = default;

    ValidationReport validate(const OpStream& ops);

private:
    Status validate_one(const Op& op);
};

}  // namespace memforge

#endif  // MEMFORGE_OP_VALIDATOR_HPP
