// MemForge - op stream.
//
// The op stream is the primary public input format. It is a sequence of typed
// records that drive the resource hub through a coherent series of mutations:
// arena appends, slab allocations, handle inserts, pin/unpin, scope begin/end,
// stats sampling, checkpoint begin/emit, and teardown. The op stream is decoded
// from raw bytes by clients and fuzz harnesses alike.

#ifndef MEMFORGE_OP_STREAM_HPP
#define MEMFORGE_OP_STREAM_HPP

#include <cstdint>
#include <vector>

#include <memforge/checked_math.hpp>
#include <memforge/error.hpp>
#include <memforge/span.hpp>

namespace memforge {

enum class OpKind : std::uint16_t {
    Noop = 0,
    ArenaAppendBlock = 1,
    PoolAlloc = 2,
    PoolFree = 3,
    HandleInsert = 4,
    HandleResolve = 5,
    EpochEnqueue = 6,
    EpochTick = 7,
    EpochDrain = 8,
    TxBegin = 9,
    TxRecord = 10,
    TxCommit = 11,
    TxRollback = 12,
    MigratePlan = 13,
    MigrateCommit = 14,
    StatsSample = 15,
    StatsFold = 16,
    CheckpointBegin = 17,
    CheckpointWriteHandleTable = 18,
    Pin = 19,
    Unpin = 20,
    PinReleaseAll = 21,
    DeferPush = 22,
    DeferCompact = 23,
    DeferFlush = 24,
    PoolCompact = 25,
    ShardRebalance = 26,
    SpanGuardAttach = 27,
    SpanGuardVerify = 28,
    BatchStage = 29,
    BatchCommit = 30,
    HubEvict = 31,
    HubDestroy = 32,
};

struct ArenaAppendOp {
    std::uint32_t block_id;
    std::uint32_t byte_count;
    std::uint32_t align;
    std::uint32_t payload_crc;
};

struct PoolAllocOp {
    std::uint32_t class_id;
    std::uint32_t slab_tag;
};

struct PoolFreeOp {
    std::uint32_t class_id;
    std::uint32_t slot_id;
};

struct HandleInsertOp {
    std::uint32_t slot_id;
    std::uint32_t generation_hint;
};

struct HandleResolveOp {
    std::uint32_t slot_id;
    std::uint32_t gen_transform;
};

struct EpochEnqueueOp {
    std::uint32_t node_id;
    std::uint32_t size_hint;
};

struct EpochTickOp {
    std::uint32_t advance;
};

struct EpochDrainOp {
    std::uint32_t cap;
};

struct TxRecordOp {
    std::uint32_t scope_id;
    std::uint32_t record_slot;
};

struct TxCommitOp {
    std::uint32_t scope_id;
};

struct TxRollbackOp {
    std::uint32_t scope_id;
};

struct MigratePlanOp {
    std::uint32_t source_pool;
    std::uint32_t dest_pool;
    std::uint32_t slot_count;
    std::uint32_t stride;
};

struct MigrateCommitOp {
    std::uint32_t source_pool;
    std::uint32_t dest_pool;
};

struct StatsSampleOp {
    std::uint32_t sample_id;
    std::uint32_t group_id;
    std::uint32_t weight;
};

struct StatsFoldOp {
    std::uint32_t bucket_id;
    std::uint32_t group_id;
};

struct CheckpointBeginOp {
    std::uint32_t snapshot_id;
    std::uint32_t entry_count;
    std::uint32_t entry_size;
};

struct CheckpointWriteHandleTableOp {
    std::uint32_t snapshot_id;
    std::uint32_t handle_count;
    std::uint32_t entry_size;
};

struct PinOp {
    std::uint32_t payload_id;
    std::uint32_t alias_count;
};

struct PinReleaseAllOp {
    std::uint32_t reserved;
};

struct DeferPushOp {
    std::uint32_t object_id;
    std::uint32_t size_hint;
};

struct DeferCompactOp {
    std::uint32_t reserved;
};

struct DeferFlushOp {
    std::uint32_t reserved;
};

struct PoolCompactOp {
    std::uint32_t class_id;
};

struct ShardRebalanceOp {
    std::uint32_t source_shard;
    std::uint32_t dest_shard;
};

struct SpanGuardAttachOp {
    std::uint32_t span_id;
    std::uint32_t guard_class;
    std::uint32_t guard_transform;
};

struct SpanGuardVerifyOp {
    std::uint32_t span_id;
};

struct BatchStageOp {
    std::uint32_t batch_id;
    std::uint32_t count;
    std::uint32_t stride;
};

struct BatchCommitOp {
    std::uint32_t batch_id;
};

struct HubEvictOp {
    std::uint32_t bucket_id;
    std::uint32_t keep_summary;
};

struct HubDestroyOp {
    std::uint32_t reserved;
};

struct Op {
    OpKind kind{OpKind::Noop};
    std::uint32_t a{0};
    std::uint32_t b{0};
    std::uint32_t c{0};
    std::uint32_t d{0};

    ArenaAppendOp asArenaAppend() const noexcept {
        return ArenaAppendOp{a, b, c, d};
    }
    PoolAllocOp asPoolAlloc() const noexcept { return PoolAllocOp{a, b}; }
    PoolFreeOp asPoolFree() const noexcept { return PoolFreeOp{a, b}; }
    HandleInsertOp asHandleInsert() const noexcept {
        return HandleInsertOp{a, b};
    }
    HandleResolveOp asHandleResolve() const noexcept {
        return HandleResolveOp{a, b};
    }
    EpochEnqueueOp asEpochEnqueue() const noexcept {
        return EpochEnqueueOp{a, b};
    }
    EpochTickOp asEpochTick() const noexcept { return EpochTickOp{a}; }
    EpochDrainOp asEpochDrain() const noexcept { return EpochDrainOp{a}; }
    TxRecordOp asTxRecord() const noexcept { return TxRecordOp{a, b}; }
    TxCommitOp asTxCommit() const noexcept { return TxCommitOp{a}; }
    TxRollbackOp asTxRollback() const noexcept { return TxRollbackOp{a}; }
    MigratePlanOp asMigratePlan() const noexcept {
        return MigratePlanOp{a, b, c, d};
    }
    MigrateCommitOp asMigrateCommit() const noexcept {
        return MigrateCommitOp{a, b};
    }
    StatsSampleOp asStatsSample() const noexcept {
        return StatsSampleOp{a, b, c};
    }
    StatsFoldOp asStatsFold() const noexcept { return StatsFoldOp{a, b}; }
    CheckpointBeginOp asCheckpointBegin() const noexcept {
        return CheckpointBeginOp{a, b, c};
    }
    CheckpointWriteHandleTableOp asCheckpointWriteHandleTable() const noexcept {
        return CheckpointWriteHandleTableOp{a, b, c};
    }
    PinOp asPin() const noexcept { return PinOp{a, b}; }
    PinReleaseAllOp asPinReleaseAll() const noexcept {
        return PinReleaseAllOp{a};
    }
    DeferPushOp asDeferPush() const noexcept { return DeferPushOp{a, b}; }
    DeferCompactOp asDeferCompact() const noexcept { return DeferCompactOp{a}; }
    DeferFlushOp asDeferFlush() const noexcept { return DeferFlushOp{a}; }
    PoolCompactOp asPoolCompact() const noexcept { return PoolCompactOp{a}; }
    ShardRebalanceOp asShardRebalance() const noexcept {
        return ShardRebalanceOp{a, b};
    }
    SpanGuardAttachOp asSpanGuardAttach() const noexcept {
        return SpanGuardAttachOp{a, b, c};
    }
    SpanGuardVerifyOp asSpanGuardVerify() const noexcept {
        return SpanGuardVerifyOp{a};
    }
    BatchStageOp asBatchStage() const noexcept {
        return BatchStageOp{a, b, c};
    }
    BatchCommitOp asBatchCommit() const noexcept { return BatchCommitOp{a}; }
    HubEvictOp asHubEvict() const noexcept { return HubEvictOp{a, b}; }
    HubDestroyOp asHubDestroy() const noexcept { return HubDestroyOp{a}; }
};

class OpStream {
public:
    OpStream() = default;

    Status decode(ConstByteSpan bytes) noexcept;
    Status encode(std::vector<std::uint8_t>& out) const noexcept;

    std::size_t size() const noexcept { return ops_.size(); }
    bool empty() const noexcept { return ops_.empty(); }
    void clear() noexcept { ops_.clear(); }

    void append(OpKind kind, std::uint32_t a = 0, std::uint32_t b = 0,
                std::uint32_t c = 0, std::uint32_t d = 0) noexcept {
        ops_.push_back(Op{kind, a, b, c, d});
    }

    void appendArenaAppend(std::uint32_t block_id, std::uint32_t byte_count,
                           std::uint32_t align, std::uint32_t payload_crc) noexcept {
        append(OpKind::ArenaAppendBlock, block_id, byte_count, align, payload_crc);
    }

    const Op& operator[](std::size_t index) const noexcept { return ops_[index]; }
    Op& operator[](std::size_t index) noexcept { return ops_[index]; }

    const Op* begin() const noexcept { return ops_.data(); }
    const Op* end() const noexcept { return ops_.data() + ops_.size(); }

private:
    std::vector<Op> ops_;
};

}  // namespace memforge

#endif  // MEMFORGE_OP_STREAM_HPP
