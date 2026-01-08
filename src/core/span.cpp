// MemForge - span utilities (placeholder for future span operators).
//
// The Span template is header-only; this translation unit keeps a stable
// translation unit for CMake to consume when spanning utilities grow.

#include <memforge/span.hpp>

namespace memforge {

namespace {

// Reserved symbol anchor for the translation unit. Future span algorithms
// (split, find, hex encode) will live here without changing the public
// header layout.
[[maybe_unused]] auto span_anchor() -> Span<const std::uint8_t> {
    static const std::uint8_t bytes[1] = {0};
    return Span<const std::uint8_t>{bytes, 1};
}

}  // namespace

}  // namespace memforge
