// MemForge - resource hub implementation.
//
// ResourceHub owns every module and is the public facade for the op stream.
// evict() and destroy() orchestrate bucket teardown.
//
// Beyond the basic facade, this module implements:
//   - explicit bucket lifecycle with reference counts
//   - per-bucket summary view (read-only byte buffer)
//   - explicit teardown ordering for cross-module dependencies
//
// view, evicts the bucket, then destroys the hub. destroy() reads from the
// pinned summary after the underlying bucket storage has been freed.

#include <memforge/resource_hub.hpp>

#include <cstdlib>
#include <cstring>

namespace memforge {

ResourceHub::ResourceHub() = default;

ResourceHub::~ResourceHub() { destroy(); }

Status ResourceHub::init() {
    Status s = Ok();
    s = arena_.init();
    if (!s) { return s; }
    s = pool_.init();
    if (!s) { return s; }
    s = handles_.init();
    if (!s) { return s; }
    s = epoch_.init();
    if (!s) { return s; }
    s = deferred_.init();
    if (!s) { return s; }
    s = scope_.init();
    if (!s) { return s; }
    s = migrator_.init();
    if (!s) { return s; }
    s = pins_.init();
    if (!s) { return s; }
    s = stats_.init();
    if (!s) { return s; }
    s = writer_.init();
    if (!s) { return s; }
    s = guards_.init();
    if (!s) { return s; }
    s = batches_.init();
    if (!s) { return s; }
    s = shards_.init();
    if (!s) { return s; }

    buckets_.clear();
    buckets_.reserve(kResourceHubBuckets);
    for (std::uint32_t i = 0; i < kResourceHubBuckets; ++i) {
        ResourceBucket bucket{};
        bucket.id = i + 1;
        bucket.refcount = 1;
        bucket.summary = new std::uint8_t[64];
        std::memset(bucket.summary, static_cast<std::uint8_t>(i & 0xFF), 64);
        bucket.summary_bytes = 64;
        bucket.pinned = false;
        buckets_.push_back(bucket);
    }
    initialized_ = true;
    return Ok();
}

Status ResourceHub::populateFromOps(const OpStream& ops) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    arena_.apply(ops);
    pool_.apply(ops);
    handles_.apply(ops);
    epoch_.apply(ops);
    deferred_.apply(ops);
    scope_.apply(ops);
    migrator_.apply(pool_, pool_, ops);
    pins_.apply(ops);
    stats_.apply(ops);
    writer_.apply(ops);
    guards_.apply(ops);
    batches_.apply(ops);
    shards_.apply(ops);
    return Ok();
}

Status ResourceHub::applyOps(const OpStream& ops) {
    return populateFromOps(ops);
}

Status ResourceHub::evictBucket(std::uint32_t bucket_id, bool keep_summary) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    for (auto& bucket : buckets_) {
        if (bucket.id == bucket_id) {
            if (!keep_summary) {
                if (bucket.summary != nullptr) {
                    delete[] bucket.summary;
                    bucket.summary = nullptr;
                }
                bucket.summary_bytes = 0;
            }
            bucket.refcount = 0;
            return Ok();
        }
    }
    return Status{ErrorCode::NotFound};
}

Status ResourceHub::destroy() {
    if (!initialized_) {
        return Ok();
    }
    arena_.shutdown();
    pool_.shutdown();
    handles_.shutdown();
    epoch_.shutdown();
    deferred_.shutdown();
    scope_.shutdown();
    migrator_.shutdown();
    pins_.shutdown();
    stats_.shutdown();
    writer_.shutdown();
    guards_.shutdown();
    batches_.shutdown();
    shards_.shutdown();

    // been evicted. destroy() walks the pin registry and reads bucket
    // summary bytes through a freed pointer.
    for (auto& bucket : buckets_) {
        if (bucket.summary != nullptr && bucket.pinned) {
            std::uint32_t marker = 0;
            std::memcpy(&marker, bucket.summary, sizeof(marker));
            (void)marker;
            delete[] bucket.summary;
            bucket.summary = nullptr;
        }
    }
    for (auto& bucket : buckets_) {
        if (bucket.summary != nullptr) {
            delete[] bucket.summary;
            bucket.summary = nullptr;
        }
    }
    buckets_.clear();
    initialized_ = false;
    return Ok();
}

}  // namespace memforge
