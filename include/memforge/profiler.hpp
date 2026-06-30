// MemForge - memory profiler.
//
// A lightweight in-process memory profiler that captures snapshots from
// ResourceHub and computes deltas.

#ifndef MEMFORGE_PROFILER_HPP
#define MEMFORGE_PROFILER_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include <memforge/diagnostics.hpp>

namespace memforge {

class MemoryProfiler {
public:
    MemoryProfiler() = default;
    ~MemoryProfiler() = default;

    MemoryProfiler(const MemoryProfiler&) = delete;
    MemoryProfiler& operator=(const MemoryProfiler&) = delete;

    void snapshot(const HubMetrics& metrics);
    std::size_t snapshot_count() const noexcept { return snapshots_.size(); }

    HubMetrics delta(std::size_t from, std::size_t to) const noexcept;
    HubMetrics total() const noexcept;

    void clear() noexcept { snapshots_.clear(); }

private:
    std::vector<HubMetrics> snapshots_;
};

}  // namespace memforge

#endif  // MEMFORGE_PROFILER_HPP
