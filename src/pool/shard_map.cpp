// MemForge - shard map implementation.
//
// ShardMap spreads SlabCache classes across N shards. rebalance() moves
// during rebalance expands the move set after the move table was sized,
// leading to a heap OOB write.
//
// Beyond the basic attach/rebalance path, this module implements:
//   - per-shard class membership tracking
//   - move table with explicit capacity
//   - safe rebalance that preserves per-class invariants

#include <memforge/shard_map.hpp>

#include <cstdlib>
#include <cstring>

namespace memforge {

ShardMap::ShardMap() = default;

ShardMap::~ShardMap() { shutdown(); }

Status ShardMap::init() {
    for (auto& shard : shards_) {
        shard = nullptr;
    }
    for (auto& list : class_membership_) {
        list.clear();
    }
    moves_.clear();
    moves_.reserve(256);
    shards_active_ = 0;
    return Ok();
}

void ShardMap::shutdown() noexcept {
    moves_.clear();
    for (auto& shard : shards_) {
        shard = nullptr;
    }
    for (auto& list : class_membership_) {
        list.clear();
    }
    shards_active_ = 0;
}

Status ShardMap::attachShard(std::uint32_t shard_id, SlabCache* cache) {
    if (shard_id >= kNumShards || cache == nullptr) {
        return Status{ErrorCode::OutOfRange};
    }
    shards_[shard_id] = cache;
    shards_active_ = std::max<std::uint32_t>(shards_active_, shard_id + 1);
    return Ok();
}

Status ShardMap::rebalance(std::uint32_t src, std::uint32_t dst) {
    if (src >= kNumShards || dst >= kNumShards || src == dst) {
        return Status{ErrorCode::InvalidArgument};
    }
    if (shards_[src] == nullptr || shards_[dst] == nullptr) {
        return Status{ErrorCode::BadState};
    }

    // Pre-count: number of classes held by source.
    std::uint32_t pre_count = static_cast<std::uint32_t>(class_membership_[src].size());

    // can add additional moves past pre_count.
    ShardMove* move_table = new ShardMove[pre_count ? pre_count : 1];
    std::uint32_t move_count = 0;
    for (std::uint32_t class_id : class_membership_[src]) {
        if (move_count >= pre_count) {
            break;
        }
        move_table[move_count++] = ShardMove{src, dst, class_id, 1};
    }
    // Post-merge: alias merge adds an extra move past pre_count.
    if (!class_membership_[src].empty() && !class_membership_[dst].empty()) {
        move_table[move_count++] = ShardMove{src, dst,
                                             class_membership_[src][0],
                                             class_membership_[dst][0]};
    }
    for (std::uint32_t i = 0; i < move_count; ++i) {
        moves_.push_back(move_table[i]);
    }
    delete[] move_table;
    return Ok();
}

Status ShardMap::apply(const OpStream& ops) {
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::ShardRebalance:
            rebalance(op.a, op.b);
            break;
        default:
            break;
        }
    }
    return Ok();
}

}  // namespace memforge
