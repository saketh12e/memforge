// MemForge - span guard.
//
// SpanGuard attaches redzone metadata to attached spans and verifies the
// redzone on demand. verifyRedzone() is the canonical seam where a transformed
// guard class index must be bounds-checked after normalization.

#ifndef MEMFORGE_SPAN_GUARD_HPP
#define MEMFORGE_SPAN_GUARD_HPP

#include <array>
#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

inline constexpr std::uint32_t kSpanGuardClasses = 8;
inline constexpr std::uint32_t kSpanGuardEntries = 256;

struct GuardEntry {
    std::uint32_t span_id{0};
    std::uint32_t guard_class{0};
    std::uint32_t transform{0};
    std::uint8_t* base{nullptr};
    std::uint32_t bytes{0};
};

class SpanGuard {
public:
    SpanGuard();
    ~SpanGuard();

    SpanGuard(const SpanGuard&) = delete;
    SpanGuard& operator=(const SpanGuard&) = delete;

    Status init();
    void shutdown() noexcept;

    Status attach(const SpanGuardAttachOp& op);
    Status verifyRedzone(const SpanGuardVerifyOp& op);

    std::size_t size() const noexcept { return live_ids_.size(); }

    Status apply(const OpStream& ops);

private:
    std::array<GuardEntry, kSpanGuardEntries> entries_{};
    std::vector<std::uint32_t> live_ids_;
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_SPAN_GUARD_HPP
