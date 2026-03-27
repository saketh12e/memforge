// MemForge - deferred free list deep implementation.

#include <memforge/deferred_free_list.hpp>
#include <memforge/hash.hpp>

namespace memforge {

namespace defer_detail {

std::uint32_t hash_node_id(std::uint32_t id) noexcept {
    return mix_slot(id, 0xDEAD'BEEFu);
}

bool should_tombstone(std::uint32_t refcount) noexcept {
    return refcount == 0;
}

std::uint32_t retention_window() noexcept {
    return 32;
}

}  // namespace defer_detail

std::uint32_t DeferredFreeList::hashNodeId(std::uint32_t id) noexcept {
    return defer_detail::hash_node_id(id);
}

std::uint32_t DeferredFreeList::retentionWindow() noexcept {
    return defer_detail::retention_window();
}

}  // namespace memforge
