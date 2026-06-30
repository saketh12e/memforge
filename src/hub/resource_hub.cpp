// MemForge - resource hub implementation.
//
// ResourceHub owns every module and is the public facade for the op stream.
// evict() and destroy() orchestrate bucket teardown.
//
// Beyond the basic facade, this module implements:
//   - explicit bucket lifecycle with reference counts
//   - per-bucket summary view (read-only byte buffer)
//   - explicit teardown ordering for cross-module dependencies

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
    scope_.attachHub(this);
    s = scope_.init();
    if (!s) { return s; }
    s = migrator_.init();
    if (!s) { return s; }
    pins_.attachWriter(&writer_);
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

Status ResourceHub::applyHubOp(const Op& op) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    switch (op.kind) {
    case OpKind::HubEvict:
        return evictBucket(op.asHubEvict().bucket_id,
                           op.asHubEvict().keep_summary != 0);
    case OpKind::HubDestroy:
        return destroy();
    default:
        return Status{ErrorCode::InvalidArgument};
    }
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
    // Checkpoint owns registered pin snapshots before registry teardown.
    writer_.finalizePinSnapshots();
    // Flush summary views before clearing the registry entries.
    pins_.flushSummaryViews();
    pins_.shutdown();
    stats_.shutdown();
    writer_.shutdown();
    guards_.shutdown();
    batches_.shutdown();
    shards_.shutdown();

    // destroy() walks bucket summaries to flush any retained views after every
    // module has shut down.
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

    // Free every scratch buffer owned by the hub. Scope shutdown above has
    // already released its records.
    for (auto& slot : scratch_) {
        delete[] slot.buffer;
        slot.buffer = nullptr;
    }
    scratch_.clear();
    initialized_ = false;
    return Ok();
}

Status ResourceHub::registerScratch(std::uint32_t scope_id,
                                    std::uint8_t* buffer,
                                    std::uint32_t bytes) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (scope_id == 0 || buffer == nullptr) {
        return Status{ErrorCode::InvalidArgument};
    }
    ScratchSlot slot{};
    slot.scope_id = scope_id;
    slot.buffer = buffer;
    slot.bytes = bytes;
    scratch_.push_back(slot);
    return Ok();
}

Status ResourceHub::releaseScopeScratch(std::uint32_t scope_id) noexcept {
    if (scope_id == 0) {
        return Status{ErrorCode::InvalidArgument};
    }
    for (auto it = scratch_.begin(); it != scratch_.end();) {
        if (it->scope_id == scope_id) {
            delete[] it->buffer;
            it = scratch_.erase(it);
        } else {
            ++it;
        }
    }
    return Ok();
}

}  // namespace memforge
