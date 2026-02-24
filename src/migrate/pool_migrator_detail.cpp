// MemForge - pool migrator deep implementation.

#include <memforge/pool_migrator.hpp>
#include <memforge/align.hpp>

namespace memforge {

namespace migrate_detail {

std::uint32_t plan_header_bytes() noexcept {
    return 32;
}

std::uint32_t per_slot_overhead() noexcept {
    return 16;
}

std::uint32_t migrate_stride(std::uint32_t count, std::uint32_t stride) noexcept {
    return plan_header_bytes() + count * (stride + per_slot_overhead());
}

}  // namespace migrate_detail

std::uint32_t PoolMigrator::planHeaderBytes() noexcept {
    return migrate_detail::plan_header_bytes();
}

std::uint32_t PoolMigrator::perSlotOverhead() noexcept {
    return migrate_detail::per_slot_overhead();
}

}  // namespace memforge
