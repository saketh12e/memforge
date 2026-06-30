// MemForge - text dump utilities for diagnostics.
//
// Helpers to format HubMetrics, op streams, and per-module state for human
// and JSON output.

#ifndef MEMFORGE_DUMP_HPP
#define MEMFORGE_DUMP_HPP

#include <cstdio>

#include <memforge/diagnostics.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

class TextDumper {
public:
    explicit TextDumper(FILE* out) : out_(out) {}
    ~TextDumper() = default;

    void dump_metrics(const HubMetrics& m);
    void dump_op(const Op& op, std::size_t index);

private:
    FILE* out_;
};

}  // namespace memforge

#endif  // MEMFORGE_DUMP_HPP
