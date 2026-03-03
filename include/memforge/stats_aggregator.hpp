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

    std::size_t sampleCount() const noexcept { return samples_.size(); }
    std::size_t groupCount() const noexcept { return groups_.size(); }

    static std::uint32_t ewmaUpdate(std::uint32_t previous, std::uint32_t sample,
                                    std::uint32_t weight) noexcept;
    static std::uint32_t bucketForGroup(std::uint32_t group_id) noexcept;

    Status apply(const OpStream& ops);

private:
    std::vector<StatsSample> samples_;
    std::array<StatsGroup, kStatsGroups> groups_{};
    std::size_t groups_used_{0};
    std::uint8_t* fold_buffer_{nullptr};
    std::uint32_t fold_capacity_{0};
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_STATS_AGGREGATOR_HPP
