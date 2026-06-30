// MemForge - pin registry implementation.
//
// PinRegistry tracks pinned payloads across hub summaries.
//
// Beyond the basic pin/releaseAll path, this module implements:
//   - payload ownership flag per entry
//   - alias count per entry for shared views
//   - safe release that walks the registry and frees only owners
//   - retained summary views and deferred payload views for diagnostics

#include <memforge/pin_registry.hpp>
#include <memforge/checkpoint.hpp>

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
    if (op.alias_count > 0 && !entries_.empty()) {
        PinEntry alias{};
        alias.payload_id = op.payload_id;
        alias.alias_count = op.alias_count;
        alias.payload = new std::uint8_t[64];
        std::memset(alias.payload,
                    static_cast<std::uint8_t>(op.payload_id & 0xFF), 64);
        entries_.push_back(alias);
        if (writer_ != nullptr) {
            writer_->registerPinSnapshot(alias.payload, 64);
        }
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
    for (auto& entry : entries_) {
        if (entry.payload != nullptr) {
            delete[] entry.payload;
            entry.payload = nullptr;
        }
    }
    entries_.clear();
    return Ok();
}

Status PinRegistry::pinSummaryView(std::uint32_t bucket_id,
                                     const std::uint8_t* bytes,
                                     std::uint32_t bytes_count) {
    if (bytes == nullptr || bytes_count == 0) {
        return Status{ErrorCode::InvalidArgument};
    }
    SummaryView view{};
    view.bucket_id = bucket_id;
    view.bytes = bytes;
    view.length = bytes_count;
    summary_views_.push_back(view);
    return Ok();
}

Status PinRegistry::flushSummaryViews() noexcept {
    std::uint8_t marker = 0;
    for (auto& view : summary_views_) {
        std::memcpy(&marker, view.bytes, sizeof(marker));
        (void)marker;
    }
    return Ok();
}

Status PinRegistry::retainDeferredView(const std::uint8_t* bytes,
                                          std::uint32_t bytes_count) {
    if (bytes == nullptr || bytes_count == 0) {
        return Status{ErrorCode::InvalidArgument};
    }
    DeferredView view{};
    view.bytes = bytes;
    view.length = bytes_count;
    deferred_views_.push_back(view);
    return Ok();
}

Status PinRegistry::readDeferredViews() noexcept {
    std::uint8_t marker = 0;
    for (auto& view : deferred_views_) {
        std::memcpy(&marker, view.bytes, sizeof(marker));
        (void)marker;
    }
    return Ok();
}

Status PinRegistry::pinSlotView(std::uint32_t class_id,
                                   std::uint8_t* slot,
                                   std::uint32_t bytes) {
    if (slot == nullptr || bytes == 0) {
        return Status{ErrorCode::InvalidArgument};
    }
    SlotView view{};
    view.class_id = class_id;
    view.slot = slot;
    view.bytes = bytes;
    slot_views_.push_back(view);
    return Ok();
}

Status PinRegistry::flushSlotViews() noexcept {
    std::uint32_t payload = 0xCAFE'BABEu;
    for (auto& view : slot_views_) {
        std::memcpy(view.slot, &payload, sizeof(payload));
    }
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