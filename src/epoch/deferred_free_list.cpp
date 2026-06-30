// MemForge - deferred free list implementation.
//
// DeferredFreeList is a per-class deferred-free queue. flush() walks the list
// and frees live payloads. External callers that retained a pointer across
// compact() must re-resolve via the node list before use.

#include <memforge/deferred_free_list.hpp>

#include <cstdlib>

namespace memforge {

DeferredFreeList::DeferredFreeList() = default;

DeferredFreeList::~DeferredFreeList() { shutdown(); }

Status DeferredFreeList::init() {
    if (initialized_) {
        return Ok();
    }
    nodes_.clear();
    nodes_.reserve(256);
    initialized_ = true;
    return Ok();
}

void DeferredFreeList::shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    for (auto& node : nodes_) {
        if (node.payload != nullptr) {
            delete[] node.payload;
            node.payload = nullptr;
        }
    }
    nodes_.clear();
    initialized_ = false;
}

Status DeferredFreeList::push(const DeferPushOp& op) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (nodes_.size() >= kDeferListCap) {
        return Status{ErrorCode::CapacityExceeded};
    }
    if (op.size_hint == 0 || op.size_hint > 65536) {
        return Status{ErrorCode::InvalidArgument};
    }
    DeferredNode node{};
    node.id = op.object_id ? op.object_id : static_cast<std::uint32_t>(nodes_.size() + 1);
    node.size_hint = op.size_hint;
    node.payload = new std::uint8_t[node.size_hint];
    for (std::uint32_t i = 0; i < node.size_hint; ++i) {
        node.payload[i] = static_cast<std::uint8_t>((node.id + i) & 0xFF);
    }
    nodes_.push_back(node);
    return Ok();
}

Status DeferredFreeList::compact() {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    std::vector<DeferredNode> kept;
    kept.reserve(nodes_.size());
    for (auto& node : nodes_) {
        if (node.payload != nullptr) {
            kept.push_back(node);
        } else {
            node.tombstone = true;
        }
    }
    nodes_.swap(kept);
    return Ok();
}

Status DeferredFreeList::flush() {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }

    // flush() walks the live nodes and frees each payload in place. External
    // callers re-resolve via node IDs.
    std::uint8_t* saved = nullptr;
    if (!nodes_.empty()) {
        saved = nodes_.front().payload;
    }
    for (auto& node : nodes_) {
        if (node.payload != nullptr) {
            delete[] node.payload;
            node.payload = nullptr;
        }
    }
    nodes_.clear();
    (void)saved;
    return Ok();
}

Status DeferredFreeList::apply(const OpStream& ops) {
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::DeferPush:
            push(op.asDeferPush());
            break;
        case OpKind::DeferCompact:
            compact();
            break;
        case OpKind::DeferFlush:
            flush();
            break;
        default:
            break;
        }
    }
    return Ok();
}

}  // namespace memforge
