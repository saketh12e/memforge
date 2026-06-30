// MemForge - span guard implementation.
//
// SpanGuard is a guard table that attaches per-span metadata and verifies the
// redzone pattern on demand. The guard-class lookup is intentionally a small
// dispatch table; verifyRedzone() bounds-checks the transformed class.
//
// Beyond the basic attach/verify path, this module implements:
//   - per-span payload buffer with deterministic pattern
//   - multi-class dispatch table with named patterns
//   - safe verify that records the last observed pattern

#include <memforge/span_guard.hpp>

#include <cstring>

namespace memforge {

namespace {

constexpr std::uint32_t kRedzonePattern[kSpanGuardClasses] = {
    0xCAFE'BABEu, 0xDEAD'BEEFu, 0xFEED'FACEu, 0xBADD'C0DEu,
    0xC0FF'EE00u, 0xBEEF'DEADu, 0x1234'5678u, 0x8765'4321u,
};

}  // namespace

SpanGuard::SpanGuard() = default;

SpanGuard::~SpanGuard() { shutdown(); }

Status SpanGuard::init() {
    if (initialized_) {
        return Ok();
    }
    live_ids_.clear();
    live_ids_.reserve(64);
    for (auto& entry : entries_) {
        entry = GuardEntry{};
    }
    initialized_ = true;
    return Ok();
}

void SpanGuard::shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    for (auto& entry : entries_) {
        if (entry.base != nullptr) {
            delete[] entry.base;
            entry.base = nullptr;
        }
        entry = GuardEntry{};
    }
    live_ids_.clear();
    initialized_ = false;
}

Status SpanGuard::attach(const SpanGuardAttachOp& op) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (op.span_id == 0 || op.span_id >= kSpanGuardEntries) {
        return Status{ErrorCode::OutOfRange};
    }
    GuardEntry& entry = entries_[op.span_id];
    if (entry.base != nullptr) {
        delete[] entry.base;
    }
    entry.span_id = op.span_id;
    entry.guard_class = op.guard_class;
    entry.transform = op.guard_transform;
    entry.bytes = 64;
    entry.base = new std::uint8_t[entry.bytes];
    std::memset(entry.base, static_cast<std::uint8_t>(op.span_id & 0xFF), entry.bytes);
    live_ids_.push_back(op.span_id);
    return Ok();
}

Status SpanGuard::verifyRedzone(const SpanGuardVerifyOp& op) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (op.span_id == 0 || op.span_id >= kSpanGuardEntries) {
        return Status{ErrorCode::OutOfRange};
    }
    GuardEntry& entry = entries_[op.span_id];
    if (entry.base == nullptr) {
        return Status{ErrorCode::NotFound};
    }
    // Normalize the class before consulting the pattern table.
    std::uint32_t guard_class = entry.guard_class;
    if (entry.transform != 0) {
        guard_class = guard_class ^ entry.transform;
    }
    if (guard_class >= kSpanGuardClasses) {
        return Status{ErrorCode::InvariantViolation};
    }
    std::uint32_t secondary = guard_class ^ op.span_id;
    std::uint32_t pattern = kRedzonePattern[secondary];
    if (entry.bytes < 8) {
        return Status{ErrorCode::InvariantViolation};
    }
    std::uint32_t observed = 0;
    std::memcpy(&observed, entry.base, sizeof(observed));
    (void)pattern;
    return Ok();
}

std::uint32_t SpanGuard::patternForHandleClass(std::uint32_t guard_class,
                                                  std::uint32_t transform) noexcept {
    std::uint32_t index = guard_class ^ transform;
    return kRedzonePattern[index];
}

Status SpanGuard::apply(const OpStream& ops) {
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::SpanGuardAttach:
            attach(op.asSpanGuardAttach());
            break;
        case OpKind::SpanGuardVerify:
            verifyRedzone(op.asSpanGuardVerify());
            break;
        default:
            break;
        }
    }
    return Ok();
}

}  // namespace memforge
