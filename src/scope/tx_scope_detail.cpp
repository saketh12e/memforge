// MemForge - tx scope deep implementation.

#include <memforge/tx_scope.hpp>
#include <memforge/hash.hpp>

namespace memforge {

namespace scope_detail {

std::uint32_t hash_scope_id(std::uint32_t scope_id) noexcept {
    return mix_slot(scope_id, 0xCAFE'BABEu);
}

bool scope_is_active(const TxScope& scope, std::uint32_t scope_id) {
    return scope.scopeCount() > 0;
}

std::uint32_t records_per_scope_limit() noexcept {
    return 256;
}

}  // namespace scope_detail

std::uint32_t TxScope::recordsPerScopeLimit() noexcept {
    return scope_detail::records_per_scope_limit();
}

}  // namespace memforge
