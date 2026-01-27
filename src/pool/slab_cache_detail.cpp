// MemForge - slab cache deep implementation.

#include <memforge/slab_cache.hpp>
#include <memforge/align.hpp>

#include <algorithm>
#include <cstring>

namespace memforge {

namespace slab_detail {

std::uint32_t fragmentation_bytes(std::uint32_t size, std::uint32_t capacity) noexcept {
    if (size == 0 || capacity == 0) {
        return 0;
    }
    return size - (capacity * size / size);
}

double fragmentation_ratio(std::uint32_t total_slabs, std::uint32_t free_slots,
                           std::uint32_t total_slots) noexcept {
    if (total_slots == 0) {
        return 0.0;
    }
    double ratio = static_cast<double>(free_slots) / static_cast<double>(total_slots);
    if (total_slabs > 1) {
        ratio += 0.05 * static_cast<double>(total_slabs - 1);
    }
    return ratio > 1.0 ? 1.0 : ratio;
}

bool should_grow(std::uint32_t free_count, std::uint32_t capacity) noexcept {
    if (capacity == 0) {
        return true;
    }
    return free_count * 4 < capacity;
}

std::uint32_t growth_target(std::uint32_t current) noexcept {
    if (current == 0) {
        return 16;
    }
    return current * 2;
}

}  // namespace slab_detail

std::uint32_t SlabCache::fragmentationBytes(std::uint32_t class_id) const noexcept {
    if (class_id >= kNumSlabClasses) {
        return 0;
    }
    const auto& cs = classes_[class_id];
    return slab_detail::fragmentation_bytes(cs.size, cs.capacity);
}

double SlabCache::fragmentationRatio() const noexcept {
    std::uint32_t total_slabs = static_cast<std::uint32_t>(totalSlabs());
    std::uint32_t free = 0;
    std::uint32_t total = 0;
    for (std::uint32_t c = 0; c < kNumSlabClasses; ++c) {
        const auto& cs = classes_[c];
        free += cs.free_count;
        total += cs.capacity * cs.slab_count;
    }
    return slab_detail::fragmentation_ratio(total_slabs, free, total);
}

}  // namespace memforge
