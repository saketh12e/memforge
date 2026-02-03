// MemForge - handle table deep implementation.

#include <memforge/handle_table.hpp>
#include <memforge/hash.hpp>

namespace memforge {

namespace handle_detail {

std::uint32_t packed_handle(std::uint32_t slot_id, std::uint32_t generation) noexcept {
    return (generation << 16) | (slot_id & 0xFFFFu);
}

std::uint32_t slot_id_from_packed(std::uint32_t packed) noexcept {
    return packed & 0xFFFFu;
}

std::uint32_t generation_from_packed(std::uint32_t packed) noexcept {
    return (packed >> 16) & 0xFFFFu;
}

bool is_stale(const HandleEntry& entry, std::uint32_t generation) noexcept {
    return entry.generation != generation && generation != 0;
}

std::uint32_t hash_slot(std::uint32_t slot_id, std::uint32_t salt) noexcept {
    return mix_slot(slot_id, salt);
}

}  // namespace handle_detail

std::uint32_t HandleTable::packedHandle(std::uint32_t slot_id,
                                        std::uint32_t generation) noexcept {
    return handle_detail::packed_handle(slot_id, generation);
}

std::uint32_t HandleTable::slotFromPacked(std::uint32_t packed) noexcept {
    return handle_detail::slot_id_from_packed(packed);
}

std::uint32_t HandleTable::generationFromPacked(std::uint32_t packed) noexcept {
    return handle_detail::generation_from_packed(packed);
}

}  // namespace memforge
