// MemForge - top-level public API surface.
//
// Pulls in the stable C++17 interfaces used by clients of the memory-resource
// stack. Internal implementation details stay under src/ and are not exported.

#ifndef MEMFORGE_API_HPP
#define MEMFORGE_API_HPP

#include <memforge/error.hpp>
#include <memforge/span.hpp>
#include <memforge/checked_math.hpp>
#include <memforge/op_stream.hpp>
#include <memforge/arena_chain.hpp>
#include <memforge/slab_cache.hpp>
#include <memforge/shard_map.hpp>
#include <memforge/handle_table.hpp>
#include <memforge/epoch_queue.hpp>
#include <memforge/deferred_free_list.hpp>
#include <memforge/tx_scope.hpp>
#include <memforge/pool_migrator.hpp>
#include <memforge/pin_registry.hpp>
#include <memforge/stats_aggregator.hpp>
#include <memforge/checkpoint.hpp>
#include <memforge/resource_hub.hpp>
#include <memforge/span_guard.hpp>
#include <memforge/batch_allocator.hpp>

namespace memforge {

inline constexpr const char* version_string() noexcept {
    return "memforge-0.4.2";
}

}  // namespace memforge

#endif  // MEMFORGE_API_HPP
