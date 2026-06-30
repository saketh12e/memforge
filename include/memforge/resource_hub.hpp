// MemForge - resource hub.
//
// The ResourceHub owns and orchestrates every other module. It is the public
// facade that the OpStream-driven harnesses call into.

#ifndef MEMFORGE_RESOURCE_HUB_HPP
#define MEMFORGE_RESOURCE_HUB_HPP

#include <array>
#include <cstdint>
#include <vector>

#include <memforge/arena_chain.hpp>
#include <memforge/batch_allocator.hpp>
#include <memforge/checkpoint.hpp>
#include <memforge/deferred_free_list.hpp>
#include <memforge/diagnostics.hpp>
#include <memforge/epoch_queue.hpp>
#include <memforge/error.hpp>
#include <memforge/handle_table.hpp>
#include <memforge/op_stream.hpp>
#include <memforge/pin_registry.hpp>
#include <memforge/pool_migrator.hpp>
#include <memforge/shard_map.hpp>
#include <memforge/slab_cache.hpp>
#include <memforge/span_guard.hpp>
#include <memforge/stats_aggregator.hpp>
#include <memforge/tx_scope.hpp>

namespace memforge {

inline constexpr std::uint32_t kResourceHubBuckets = 8;

struct ResourceBucket {
    std::uint32_t id{0};
    std::uint32_t refcount{0};
    std::uint8_t* summary{nullptr};
    std::uint32_t summary_bytes{0};
    bool pinned{false};
};

class ResourceHub {
public:
    ResourceHub();
    ~ResourceHub();

    ResourceHub(const ResourceHub&) = delete;
    ResourceHub& operator=(const ResourceHub&) = delete;

    Status init();
    void shutdown() noexcept;

    Status populateFromOps(const OpStream& ops);
    Status applyOps(const OpStream& ops);
    Status evictBucket(std::uint32_t bucket_id, bool keep_summary);
    Status destroy();

    // Apply a single op directly. Used by replay workflows to dispatch
    // HubEvict/HubDestroy outside the applyOps ordering.
    Status applyHubOp(const Op& op);

    // Register a scratch buffer against a transactional scope. The buffer is
    // owned by the hub and released by releaseScopeScratch() or hub shutdown.
    // TxScope uses this so that rollback can free a buffer through the hub
    // and not through its own record list.
    Status registerScratch(std::uint32_t scope_id, std::uint8_t* buffer,
                           std::uint32_t bytes);
    // Release and free every scratch buffer registered under this scope_id.
    Status releaseScopeScratch(std::uint32_t scope_id) noexcept;
    std::size_t scratchCount() const noexcept { return scratch_.size(); }

    ArenaChain& arena() noexcept { return arena_; }
    SlabCache& pool() noexcept { return pool_; }
    HandleTable& handles() noexcept { return handles_; }
    EpochQueue& epoch() noexcept { return epoch_; }
    DeferredFreeList& deferred() noexcept { return deferred_; }
    TxScope& scope() noexcept { return scope_; }
    PoolMigrator& migrator() noexcept { return migrator_; }
    PinRegistry& pins() noexcept { return pins_; }
    StatsAggregator& stats() noexcept { return stats_; }
    CheckpointWriter& writer() noexcept { return writer_; }
    SpanGuard& guards() noexcept { return guards_; }
    BatchAllocator& batches() noexcept { return batches_; }
    ShardMap& shards() noexcept { return shards_; }

    std::size_t bucketCount() const noexcept { return buckets_.size(); }
    const ResourceBucket& bucket(std::size_t index) const noexcept {
        return buckets_[index];
    }

    static std::uint32_t bucketCountLimit() noexcept;
    static std::uint32_t summaryBytesPerBucket() noexcept;
    static std::uint32_t bucketAlignment() noexcept;

    HubMetrics collectMetrics() const noexcept;

private:
    ArenaChain arena_;
    SlabCache pool_;
    HandleTable handles_;
    EpochQueue epoch_;
    DeferredFreeList deferred_;
    TxScope scope_;
    PoolMigrator migrator_;
    PinRegistry pins_;
    StatsAggregator stats_;
    CheckpointWriter writer_;
    SpanGuard guards_;
    BatchAllocator batches_;
    ShardMap shards_;
    std::vector<ResourceBucket> buckets_;
    struct ScratchSlot {
        std::uint32_t scope_id{0};
        std::uint8_t* buffer{nullptr};
        std::uint32_t bytes{0};
    };
    std::vector<ScratchSlot> scratch_;
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_RESOURCE_HUB_HPP
