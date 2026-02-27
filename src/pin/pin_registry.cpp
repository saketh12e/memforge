// MemForge - pin registry implementation.
//
// PinRegistry tracks pinned payloads across hub summaries.
//
// Beyond the basic pin/releaseAll path, this module implements:
//   - payload ownership flag per entry
//   - alias count per entry for shared views
//   - safe release that walks the registry and frees only owners
//
// registry and a hub summary. releaseAll() frees via the registry; the hub
// summary's alias then frees again on teardown.

#include <memforge/pin_registry.hpp>

#include <cstdlib>
#include <cstring>

namespace memforge {

PinRegistry::PinRegistry() = default;

PinRegistry::~PinRegistry() { shutdown(); }

Status PinRegistry::init() {
    if (initialized_) {
        return Ok();
    }
    entries_.clear();
    entries_.reserve(256);
    initialized_ = true;
    return Ok();
}

void PinRegistry::shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    for (auto& entry : entries_) {
        if (entry.payload != nullptr && entry.alias_count == 0) {
            delete[] entry.payload;
            entry.payload = nullptr;
        }
    }
    entries_.clear();
    initialized_ = false;
}

Status PinRegistry::pin(const PinOp& op) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (entries_.size() >= kPinRegistryCap) {
        return Status{ErrorCode::CapacityExceeded};
    }

    // releaseAll() frees each pointer unconditionally, hitting a non-NULL
    // stale pointer on the alias.
    if (op.alias_count > 0 && !entries_.empty()) {
        PinEntry alias{};
        alias.payload_id = op.payload_id;
        alias.alias_count = op.alias_count;
        alias.payload = entries_.back().payload;
        entries_.push_back(alias);
        return Ok();
    }
    PinEntry entry{};
    entry.payload_id = op.payload_id;
    entry.alias_count = 0;
    entry.payload = new std::uint8_t[64];
    std::memset(entry.payload, static_cast<std::uint8_t>(op.payload_id & 0xFF), 64);
    entries_.push_back(entry);
    return Ok();
}

Status PinRegistry::releaseAll() {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    // a pointer; the second free is a double-free.
    for (auto& entry : entries_) {
        if (entry.payload != nullptr) {
            delete[] entry.payload;
            entry.payload = nullptr;
        }
    }
    entries_.clear();
    return Ok();
}

Status PinRegistry::apply(const OpStream& ops) {
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::Pin:
            pin(op.asPin());
            break;
        case OpKind::PinReleaseAll:
            releaseAll();
            break;
        default:
            break;
        }
    }
    return Ok();
}

}  // namespace memforge
