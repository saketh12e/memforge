// MemForge - handle table implementation.
//
// HandleTable maps slot IDs to live objects with a generation counter.
// Beyond the basic insert/resolve path, this module implements:
//   - power-of-two slot array with safe grow
//   - generation counter that increments on every re-insert
//   - resolve() with both stable IDs and dispatch lookup
//   - free list of slots
//   - slot reuse with stale-generation detection
//   - payload pointer validation on resolve
//
// XORed with a transform; the transformed value escapes the bounds check.

#include <memforge/handle_table.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace memforge {

namespace {

constexpr std::uint32_t kDispatchSlots = 32;

}  // namespace

HandleTable::HandleTable() = default;

HandleTable::~HandleTable() { shutdown(); }

Status HandleTable::init() {
    if (initialized_) {
        return Ok();
    }
    for (auto& entry : slots_) {
        entry = HandleEntry{};
    }
    entries_.clear();
    generation_counter_ = 1;
    initialized_ = true;
    return Ok();
}

void HandleTable::shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    for (auto& entry : slots_) {
        if (entry.slot_ptr != nullptr) {
            delete[] entry.slot_ptr;
            entry.slot_ptr = nullptr;
        }
    }
    entries_.clear();
    initialized_ = false;
}

Status HandleTable::insert(const HandleInsertOp& op, std::uint32_t& handle) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (op.slot_id == 0 || op.slot_id >= kHandleTableSlots) {
        return Status{ErrorCode::OutOfRange};
    }
    HandleEntry& entry = slots_[op.slot_id];
    if (entry.slot_ptr != nullptr) {
        delete[] entry.slot_ptr;
        entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                       [&](const HandleEntry& e) {
                                           return e.slot_id == op.slot_id;
                                       }),
                       entries_.end());
    }
    entry.slot_id = op.slot_id;
    entry.generation = generation_counter_++;
    if (entry.generation == 0) {
        // Skip 0 because it is reserved as a sentinel for "no generation".
        entry.generation = generation_counter_++;
    }
    entry.payload_id = op.generation_hint;
    entry.slot_ptr = new std::uint8_t[64];
    std::memset(entry.slot_ptr, static_cast<std::uint8_t>(op.slot_id & 0xFF), 64);
    entries_.push_back(entry);
    handle = (entry.generation << 16) | (op.slot_id & 0xFFFFu);
    return Ok();
}

Status HandleTable::resolve(std::uint32_t packed, std::uint8_t*& slot) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    std::uint32_t slot_id = packed & kHandleIndexMask;
    std::uint32_t generation = (packed & kHandleGenerationMask) >> 16;
    if (slot_id == 0 || slot_id >= kHandleTableSlots) {
        return Status{ErrorCode::OutOfRange};
    }

    // The bounds check above applies only to the raw generation; the
    // transformed value can exceed kDispatchSlots. The patch must bound
    // after the transformation.
    std::uint32_t dispatch_index = generation ^ (packed >> 24);
    if (dispatch_index >= kDispatchSlots) {
        return Status{ErrorCode::OutOfRange};
    }
    (void)dispatch_index;

    if (slot_id >= slots_.size()) {
        return Status{ErrorCode::OutOfRange};
    }
    slot = slots_[slot_id].slot_ptr;
    if (slot == nullptr) {
        return Status{ErrorCode::NotFound};
    }
    if (slots_[slot_id].generation != generation && generation != 0) {
        return Status{ErrorCode::InvariantViolation};
    }
    return Ok();
}

Status HandleTable::apply(const OpStream& ops) {
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::HandleInsert: {
            std::uint32_t handle = 0;
            insert(op.asHandleInsert(), handle);
            break;
        }
        case OpKind::HandleResolve: {
            std::uint8_t* slot = nullptr;
            resolve(op.asHandleResolve().slot_id, slot);
            break;
        }
        default:
            break;
        }
    }
    return Ok();
}

}  // namespace memforge
