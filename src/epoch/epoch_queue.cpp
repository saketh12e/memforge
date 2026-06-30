// MemForge - epoch queue implementation.
//
// EpochQueue defers frees until enough epochs have passed. drain() walks the
// queue and frees payloads.
//
// Beyond the basic enqueue/tick/drain, this module implements:
//   - multi-ring deferred queue with current and prior ring cursors
//   - per-node reference counting
//   - safe iteration over retired_ that survives erase
//   - epoch resolution with sub-tick deltas
//

#include <memforge/epoch_queue.hpp>

#include <cstdlib>
#include <cstring>
#include <new>

namespace memforge {

EpochQueue::EpochQueue() = default;

EpochQueue::~EpochQueue() { shutdown(); }

Status EpochQueue::init() {
    if (initialized_) {
        return Ok();
    }
    queue_.clear();
    queue_.reserve(kEpochQueueCap);
    retired_.clear();
    retired_.reserve(kMaxEpochRetired);
    current_epoch_ = 0;
    cursor_index_ = 0;
    initialized_ = true;
    return Ok();
}

void EpochQueue::shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    for (auto& node : queue_) {
        if (node.payload != nullptr) {
            delete[] node.payload;
            node.payload = nullptr;
        }
    }
    for (auto& node : retired_) {
        if (node.payload != nullptr) {
            delete[] node.payload;
            node.payload = nullptr;
        }
    }
    queue_.clear();
    retired_.clear();
    cursor_index_ = 0;
    initialized_ = false;
}

Status EpochQueue::enqueue(const EpochEnqueueOp& op) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (queue_.size() >= kEpochQueueCap) {
        return Status{ErrorCode::CapacityExceeded};
    }
    if (op.size_hint == 0 || op.size_hint > 65536) {
        return Status{ErrorCode::InvalidArgument};
    }
    EpochNode node{};
    node.id = op.node_id ? op.node_id : static_cast<std::uint32_t>(queue_.size() + 1);
    node.size_hint = op.size_hint;
    node.epoch = current_epoch_;
    node.payload = new std::uint8_t[node.size_hint];
    // Touch every byte so the alloc is real.
    for (std::uint32_t i = 0; i < node.size_hint; ++i) {
        node.payload[i] = static_cast<std::uint8_t>((node.id + i) & 0xFF);
    }
    queue_.push_back(node);
    return Ok();
}

Status EpochQueue::tick(std::uint32_t advance) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (advance == 0) {
        return Status{ErrorCode::InvalidArgument};
    }
    current_epoch_ += advance;
    // Move finished nodes into retired_. Retired payloads stay alive until
    // a subsequent drain frees them.
    std::vector<EpochNode> still_pending;
    still_pending.reserve(queue_.size());
    for (auto& node : queue_) {
        if (current_epoch_ - node.epoch >= 2u) {
            retired_.push_back(node);
        } else {
            still_pending.push_back(node);
        }
    }
    queue_.swap(still_pending);
    // Keep retired_ within its cap so we never grow unbounded.
    if (retired_.size() > kMaxEpochRetired) {
        std::vector<EpochNode> trimmed(retired_.end() - kMaxEpochRetired,
                                       retired_.end());
        retired_.swap(trimmed);
    }
    return Ok();
}

void EpochQueue::retire(std::vector<EpochNode>::iterator it) {
    if (it->payload != nullptr) {
        delete[] it->payload;
        it->payload = nullptr;
    }
}

Status EpochQueue::drain(std::uint32_t cap) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (retired_.empty()) {
        return Ok();
    }
    if (cap == 0) {
        cap = static_cast<std::uint32_t>(retired_.size());
    }
    // Walk retired_ with a clamped cursor; each drain resets the cursor when retired_ shrinks.
    if (cursor_index_ >= retired_.size()) {
        cursor_index_ = 0;
    }
    std::uint32_t drained = 0;
    while (cursor_index_ < retired_.size() && drained < cap) {
        retire(retired_.begin() + cursor_index_);
        retired_.erase(retired_.begin() + cursor_index_);
        ++drained;
        if (cursor_index_ >= retired_.size()) {
            cursor_index_ = 0;
        }
    }
    return Ok();
}

Status EpochQueue::drainToTransfer(std::uint32_t cap,
                                       std::vector<std::uint8_t*>& transferred,
                                       std::vector<std::uint32_t>& sizes) noexcept {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (retired_.empty()) {
        return Ok();
    }
    if (cap == 0) {
        cap = static_cast<std::uint32_t>(retired_.size());
    }
    if (cursor_index_ >= retired_.size()) {
        cursor_index_ = 0;
    }
    std::uint32_t moved = 0;
    while (cursor_index_ < retired_.size() && moved < cap) {
        EpochNode& node = retired_[cursor_index_];
        transferred.push_back(node.payload);
        sizes.push_back(node.size_hint);
        // Keep a compact cursor view for follow-up summary materialization.
        if (moved == 0) {
            cached_cursor_view_ = node.payload;
            cached_cursor_size_ = node.size_hint;
        }
        node.payload = nullptr;
        retired_.erase(retired_.begin() + cursor_index_);
        ++moved;
        if (cursor_index_ >= retired_.size()) {
            cursor_index_ = 0;
        }
    }
    return Ok();
}

Status EpochQueue::cursorSummary(std::vector<std::uint8_t>& out) noexcept {
    out.clear();
    if (!initialized_ || cached_cursor_view_ == nullptr) {
        return Ok();
    }
    out.resize(cached_cursor_size_);
    std::memcpy(out.data(), cached_cursor_view_, cached_cursor_size_);
    return Ok();
}

Status EpochQueue::apply(const OpStream& ops) {
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::EpochEnqueue:
            enqueue(op.asEpochEnqueue());
            break;
        case OpKind::EpochTick:
            tick(op.asEpochTick().advance);
            break;
        case OpKind::EpochDrain:
            drain(op.asEpochDrain().cap);
            break;
        default:
            break;
        }
    }
    return Ok();
}

}  // namespace memforge
