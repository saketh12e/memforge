// MemForge - stats aggregator deep implementation.

#include <memforge/stats_aggregator.hpp>
#include <memforge/hash.hpp>

namespace memforge {

namespace stats_detail {

std::uint32_t ewma_update(std::uint32_t previous, std::uint32_t sample,
                          std::uint32_t weight) noexcept {
    if (weight == 0) {
        return previous;
    }
    std::uint64_t sum = static_cast<std::uint64_t>(previous) * (weight - 1) + sample;
    return static_cast<std::uint32_t>(sum / weight);
}

std::uint32_t hash_group_id(std::uint32_t group_id) noexcept {
    return mix_slot(group_id, 0xC0FF'EE00u);
}

std::uint32_t bucket_for_group(std::uint32_t group_id) noexcept {
    return group_id & 0x0Fu;
}

}  // namespace stats_detail

std::uint32_t StatsAggregator::ewmaUpdate(std::uint32_t previous, std::uint32_t sample,
                                          std::uint32_t weight) noexcept {
    return stats_detail::ewma_update(previous, sample, weight);
}

std::uint32_t StatsAggregator::bucketForGroup(std::uint32_t group_id) noexcept {
    return stats_detail::bucket_for_group(group_id);
}

}  // namespace memforge
