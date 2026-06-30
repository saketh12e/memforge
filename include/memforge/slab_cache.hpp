// MemForge - slab cache.
//
// The SlabCache manages per-class free lists, slab metadata, and growth
// growClass() is the canonical entrypoint for class growth; alloc()
// is the canonical entrypoint for slot allocation. compact() builds a
// relocation table to move live slots together.

#ifndef MEMFORGE_SLAB_CACHE_HPP
#define MEMFORGE_SLAB_CACHE_HPP

#include <array>
#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

inline constexpr std::uint32_t kNumSlabClasses = 6;
inline constexpr std::uint32_t kMaxSlotsPerClass = 4096;
inline constexpr std::uint32_t kMaxSlabs = 1024;

struct SlabClassInfo {
    std::uint32_t size{0};
    std::uint32_t capacity{0};
    std::uint32_t free_count{0};
    std::uint32_t slab_count{0};
};

struct SlabSlot {
    std::uint32_t id{0};
    std::uint8_t* base{nullptr};
    std::uint32_t generation{0};
};

class SlabCache {
public:
    SlabCache();
    ~SlabCache();

    SlabCache(const SlabCache&) = delete;
    SlabCache& operator=(const SlabCache&) = delete;

    Status init();
    void shutdown() noexcept;

    Status alloc(std::uint32_t class_id, SlabSlot& slot);
    Status freeSlot(std::uint32_t class_id, std::uint32_t slot_id);
    Status growClass(std::uint32_t class_id);
    Status compact(std::uint32_t class_id);

    SlabClassInfo classInfo(std::uint32_t class_id) const noexcept;

    std::size_t totalSlabs() const noexcept;
    std::uint32_t generationOf(std::uint32_t class_id, std::uint32_t slot_id) const noexcept;

    std::uint32_t fragmentationBytes(std::uint32_t class_id) const noexcept;
    double fragmentationRatio() const noexcept;

    Status apply(const OpStream& ops);

private:
    Status ensureClass(std::uint32_t class_id);
    void releaseClass(std::uint32_t class_id) noexcept;

    struct ClassState {
        std::vector<std::uint8_t*> slabs;
        std::vector<std::uint32_t> free_slots;
        std::vector<std::uint32_t> generations;
        std::uint32_t size{0};
        std::uint32_t capacity{0};
        std::uint32_t free_count{0};
        std::uint32_t slab_count{0};
    };

    std::array<ClassState, kNumSlabClasses> classes_;
    std::uint32_t next_slab_id_{0};
    std::uint32_t next_slot_id_{0};
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_SLAB_CACHE_HPP
