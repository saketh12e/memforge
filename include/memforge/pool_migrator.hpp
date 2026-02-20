// MemForge - cross-pool migration planner and committer.

#ifndef MEMFORGE_POOL_MIGRATOR_HPP
#define MEMFORGE_POOL_MIGRATOR_HPP

#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>
#include <memforge/slab_cache.hpp>

namespace memforge {

inline constexpr std::uint32_t kMaxMigrationSlots = 4096;
inline constexpr std::uint32_t kMigrationHeaderSize = 32;

struct MigrationSlot {
    std::uint32_t slot_id{0};
    std::uint8_t* source{nullptr};
    std::uint8_t* dest{nullptr};
    std::uint32_t generation{0};
};

class PoolMigrator {
public:
    PoolMigrator();
    ~PoolMigrator();

    PoolMigrator(const PoolMigrator&) = delete;
    PoolMigrator& operator=(const PoolMigrator&) = delete;

    Status init();
    void shutdown() noexcept;

    Status plan(SlabCache& source, SlabCache& dest, const MigratePlanOp& op);
    Status commit(SlabCache& source, SlabCache& dest);

    std::size_t planSize() const noexcept { return staged_.size(); }

    static std::uint32_t planHeaderBytes() noexcept;
    static std::uint32_t perSlotOverhead() noexcept;

    Status apply(SlabCache& source, SlabCache& dest, const OpStream& ops);

private:
    std::vector<MigrationSlot> staged_;
    std::uint32_t stride_{0};
    std::uint8_t* header_{nullptr};
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_POOL_MIGRATOR_HPP
