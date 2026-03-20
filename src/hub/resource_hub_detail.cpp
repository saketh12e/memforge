// MemForge - resource hub deep implementation.

#include <memforge/resource_hub.hpp>
#include <memforge/align.hpp>

namespace memforge {

namespace hub_detail {

std::uint32_t bucket_count() noexcept {
    return 8;
}

std::uint32_t summary_bytes_per_bucket() noexcept {
    return 64;
}

std::uint32_t bucket_alignment() noexcept {
    return 8;
}

}  // namespace hub_detail

std::uint32_t ResourceHub::bucketCountLimit() noexcept {
    return hub_detail::bucket_count();
}

std::uint32_t ResourceHub::summaryBytesPerBucket() noexcept {
    return hub_detail::summary_bytes_per_bucket();
}

std::uint32_t ResourceHub::bucketAlignment() noexcept {
    return hub_detail::bucket_alignment();
}

HubMetrics ResourceHub::collectMetrics() const noexcept {
    HubMetrics m{};
    m.arena.blocks = static_cast<std::uint32_t>(arena_.blockCount());
    m.arena.total_bytes = arena_.finalizedBytes();
    m.arena.fragmented_bytes = arena_.fragmentedBytes();
    m.arena.alignment_waste = m.arena.fragmented_bytes;

    m.pool.classes = kNumSlabClasses;
    m.pool.slabs = static_cast<std::uint32_t>(pool_.totalSlabs());
    m.pool.free_slots = 0;
    m.pool.total_slots = 0;
    for (std::uint32_t c = 0; c < kNumSlabClasses; ++c) {
        auto info = pool_.classInfo(c);
        m.pool.free_slots += info.free_count;
        m.pool.total_slots += info.capacity * info.slab_count;
    }
    m.pool.fragmentation_ratio = pool_.fragmentationRatio();

    m.handles.live = static_cast<std::uint32_t>(handles_.size());
    m.handles.stale = 0;
    m.handles.generations = 0;

    m.epoch.current = epoch_.epoch();
    m.epoch.pending = static_cast<std::uint32_t>(epoch_.pending());
    m.epoch.retired = 0;

    m.pins.entries = static_cast<std::uint32_t>(pins_.size());
    m.pins.aliases = 0;

    m.checkpoint.bytes = static_cast<std::uint32_t>(writer_.byteSize());
    m.checkpoint.sections = 0;

    m.buckets = static_cast<std::uint32_t>(buckets_.size());
    return m;
}

}  // namespace memforge
