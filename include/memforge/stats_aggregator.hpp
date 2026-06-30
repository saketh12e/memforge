// MemForge - stats aggregator.

#ifndef MEMFORGE_STATS_AGGREGATOR_HPP
#define MEMFORGE_STATS_AGGREGATOR_HPP

#include <array>
#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

inline constexpr std::uint32_t kStatsBuckets = 16;
inline constexpr std::uint32_t kStatsGroups = 16;

struct StatsSample {
    std::uint32_t sample_id{0};
    std::uint32_t group_id{0};
    std::uint32_t weight{0};
};

struct StatsGroup {
    std::uint32_t group_id{0};
    std::uint64_t total_weight{0};
    std::uint64_t total_samples{0};
};

class StatsAggregator {
public:
    StatsAggregator();
    ~StatsAggregator();

    StatsAggregator(const StatsAggregator&) = delete;
    StatsAggregator& operator=(const StatsAggregator&) = delete;

    Status init();
    void shutdown() noexcept;

    Status sample(const StatsSampleOp& op);
    Status foldMetrics(const StatsFoldOp& op);

    // Record a shard move into a pre-sized move buffer. Used by
    // ShardMap::rebalance() as a callback.
    void recordShardMove(std::uint32_t class_id, std::uint32_t shard_id);
    std::size_t moveBufferCount() const noexcept {
        return move_buffer_count_;
    }
    std::uint32_t moveBufferCapacity() const noexcept {
        return move_buffer_capacity_;
    }
    // Pre-size the move buffer to capacity. The buffer is fixed-size; callers
    // should size it appropriately before issuing recordShardMove() calls.
    Status prepareMoveBuffer(std::uint32_t capacity) noexcept;

    // Number of StatsGroup entries that the most recent foldMetrics() wrote
    // into fold_buffer_. Used by CheckpointWriter to size its stats section.
    std::size_t foldPlanSize() const noexcept { return fold_plan_.size(); }
    const std::vector<std::uint32_t>& foldPlanGroupIds() const noexcept {
        return fold_plan_;
    }

    std::size_t sampleCount() const noexcept { return samples_.size(); }
    std::size_t groupCount() const noexcept { return groups_.size(); }

    static std::uint32_t ewmaUpdate(std::uint32_t previous, std::uint32_t sample,
                                    std::uint32_t weight) noexcept;
    static std::uint32_t bucketForGroup(std::uint32_t group_id) noexcept;

    Status apply(const OpStream& ops);

private:
    struct MoveEntry {
        std::uint32_t class_id{0};
        std::uint32_t shard_id{0};
    };
    std::vector<StatsSample> samples_;
    std::array<StatsGroup, kStatsGroups> groups_{};
    std::size_t groups_used_{0};
    std::uint8_t* fold_buffer_{nullptr};
    std::uint32_t fold_capacity_{0};
    std::vector<std::uint32_t> fold_plan_;
    MoveEntry* move_buffer_{nullptr};
    std::uint32_t move_buffer_capacity_{0};
    std::uint32_t move_buffer_count_{0};
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_STATS_AGGREGATOR_HPP
