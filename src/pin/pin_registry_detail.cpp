// MemForge - pin registry deep implementation.

#include <memforge/pin_registry.hpp>
#include <memforge/hash.hpp>

namespace memforge {

namespace pin_detail {

std::uint32_t hash_payload_id(std::uint32_t payload_id) noexcept {
    return mix_slot(payload_id, 0xFEED'FACEu);
}

bool is_alias_entry(const PinEntry& entry) noexcept {
    return entry.alias_count > 0;
}

std::uint32_t alias_limit() noexcept {
    return 256;
}

}  // namespace pin_detail

std::uint32_t PinRegistry::hashPayloadId(std::uint32_t payload_id) noexcept {
    return pin_detail::hash_payload_id(payload_id);
}

std::uint32_t PinRegistry::aliasLimit() noexcept {
    return pin_detail::alias_limit();
}

}  // namespace memforge
