// MemForge - shard map.
//
// A ShardMap spreads SlabCache classes across N shards. rebalance() moves
// classes between shards and emits a relocation table.

#ifndef MEMFORGE_SHARD_MAP_HPP
#define MEMFORGE_SHARD_MAP_HPP

#include <array>
#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>
#include <memforge/slab_cache.hpp>

namespace memforge {

inline constexpr std::uint32_t kNumShards = 4;
inline constexpr std::uint32_t kMaxShardMoves = 1024;

struct ShardMove {
    std::uint32_t src_shard{0};
    std::uint32_t dst_shard{0};
    std::uint32_t class_id{0};
    std::uint32_t slot_count{0};
};

class StatsAggregator;

class ShardMap {
public:
    ShardMap();
    ~ShardMap();

    ShardMap(const ShardMap&) = delete;
    ShardMap& operator=(const ShardMap&) = delete;

    void setStatsCallback(StatsAggregator* stats) noexcept {
        stats_callback_ = stats;
    }
    Status init();
    void shutdown() noexcept;

    Status attachShard(std::uint32_t shard_id, SlabCache* cache);
    Status registerClassMembership(std::uint32_t shard_id,
                                    std::uint32_t class_id);
    Status rebalance(std::uint32_t src, std::uint32_t dst);

    // Returns the list of shards that currently claim ownership of class_id.
    // Multiple shards can register the same class during class migration,
    // and the alias list is computed lazily on demand.
    std::vector<std::uint32_t> classAliasesForBucket(
        std::uint32_t class_id) const;

    std::size_t shardCount() const noexcept { return shards_active_; }
    const std::vector<ShardMove>& moves() const noexcept { return moves_; }

    Status apply(const OpStream& ops);

private:
    std::array<SlabCache*, kNumShards> shards_{};
    std::array<std::vector<std::uint32_t>, kNumShards> class_membership_{};
    std::vector<ShardMove> moves_;
    StatsAggregator* stats_callback_{nullptr};
    std::uint32_t shards_active_{0};
};

}  // namespace memforge

#endif  // MEMFORGE_SHARD_MAP_HPP
