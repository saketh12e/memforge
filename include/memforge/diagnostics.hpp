// MemForge - diagnostics and metrics structures.

#ifndef MEMFORGE_DIAGNOSTICS_HPP
#define MEMFORGE_DIAGNOSTICS_HPP

#include <cstdint>

namespace memforge {

struct ArenaMetrics {
    std::uint32_t blocks{0};
    std::uint32_t total_bytes{0};
    std::uint32_t fragmented_bytes{0};
    std::uint32_t alignment_waste{0};
};

struct PoolMetrics {
    std::uint32_t classes{0};
    std::uint32_t slabs{0};
    std::uint32_t free_slots{0};
    std::uint32_t total_slots{0};
    double fragmentation_ratio{0.0};
};

struct HandleMetrics {
    std::uint32_t live{0};
    std::uint32_t obsolete{0};
    std::uint32_t generations{0};
};

struct EpochMetrics {
    std::uint32_t current{0};
    std::uint32_t pending{0};
    std::uint32_t retired{0};
};

struct PinMetrics {
    std::uint32_t entries{0};
    std::uint32_t aliases{0};
};

struct CheckpointMetrics {
    std::uint32_t bytes{0};
    std::uint32_t sections{0};
};

struct HubMetrics {
    ArenaMetrics arena;
    PoolMetrics pool;
    HandleMetrics handles;
    EpochMetrics epoch;
    PinMetrics pins;
    CheckpointMetrics checkpoint;
    std::uint32_t buckets{0};
};

}  // namespace memforge

#endif  // MEMFORGE_DIAGNOSTICS_HPP
