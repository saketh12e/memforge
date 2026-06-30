// MemForge - memory profiler implementation.

#include <memforge/profiler.hpp>

#include <algorithm>

namespace memforge {

void MemoryProfiler::snapshot(const HubMetrics& metrics) {
    snapshots_.push_back(metrics);
}

HubMetrics MemoryProfiler::delta(std::size_t from, std::size_t to) const noexcept {
    if (snapshots_.empty() || from >= snapshots_.size() || to >= snapshots_.size()) {
        return HubMetrics{};
    }
    if (to < from) {
        std::swap(from, to);
    }
    const HubMetrics& a = snapshots_[from];
    const HubMetrics& b = snapshots_[to];
    HubMetrics d{};
    d.arena.blocks = b.arena.blocks - a.arena.blocks;
    d.arena.total_bytes = b.arena.total_bytes - a.arena.total_bytes;
    d.arena.fragmented_bytes = b.arena.fragmented_bytes - a.arena.fragmented_bytes;
    d.pool.classes = b.pool.classes;
    d.pool.slabs = b.pool.slabs - a.pool.slabs;
    d.pool.free_slots = b.pool.free_slots - a.pool.free_slots;
    d.pool.total_slots = b.pool.total_slots - a.pool.total_slots;
    d.pool.fragmentation_ratio = b.pool.fragmentation_ratio - a.pool.fragmentation_ratio;
    d.handles.live = b.handles.live - a.handles.live;
    d.handles.obsolete = b.handles.obsolete - a.handles.obsolete;
    d.epoch.current = b.epoch.current - a.epoch.current;
    d.epoch.pending = b.epoch.pending - a.epoch.pending;
    d.epoch.retired = b.epoch.retired - a.epoch.retired;
    d.pins.entries = b.pins.entries - a.pins.entries;
    d.checkpoint.bytes = b.checkpoint.bytes - a.checkpoint.bytes;
    d.buckets = b.buckets;
    return d;
}

HubMetrics MemoryProfiler::total() const noexcept {
    HubMetrics t{};
    if (snapshots_.empty()) {
        return t;
    }
    t = snapshots_.front();
    for (std::size_t i = 1; i < snapshots_.size(); ++i) {
        const HubMetrics& s = snapshots_[i];
        t.arena.blocks += s.arena.blocks;
        t.arena.total_bytes += s.arena.total_bytes;
        t.arena.fragmented_bytes += s.arena.fragmented_bytes;
        t.pool.slabs += s.pool.slabs;
        t.pool.free_slots += s.pool.free_slots;
        t.pool.total_slots += s.pool.total_slots;
        t.handles.live += s.handles.live;
        t.epoch.pending += s.epoch.pending;
        t.pins.entries += s.pins.entries;
        t.checkpoint.bytes += s.checkpoint.bytes;
    }
    t.buckets = snapshots_.front().buckets;
    return t;
}

}  // namespace memforge
