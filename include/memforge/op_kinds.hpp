// MemForge - op stream op kinds reference.
//
// Centralized listing of all op kinds for documentation and tooling.

#ifndef MEMFORGE_OP_KINDS_HPP
#define MEMFORGE_OP_KINDS_HPP

#include <cstdint>

#include <memforge/op_stream.hpp>

namespace memforge {

inline constexpr const char* op_kind_name(OpKind kind) noexcept {
    switch (kind) {
    case OpKind::Noop:                       return "noop";
    case OpKind::ArenaAppendBlock:           return "arena_append";
    case OpKind::PoolAlloc:                  return "pool_alloc";
    case OpKind::PoolFree:                   return "pool_free";
    case OpKind::HandleInsert:               return "handle_insert";
    case OpKind::HandleResolve:              return "handle_resolve";
    case OpKind::EpochEnqueue:               return "epoch_enqueue";
    case OpKind::EpochTick:                  return "epoch_tick";
    case OpKind::EpochDrain:                 return "epoch_drain";
    case OpKind::TxBegin:                    return "tx_begin";
    case OpKind::TxRecord:                   return "tx_record";
    case OpKind::TxCommit:                   return "tx_commit";
    case OpKind::TxRollback:                 return "tx_rollback";
    case OpKind::MigratePlan:                return "migrate_plan";
    case OpKind::MigrateCommit:              return "migrate_commit";
    case OpKind::StatsSample:                return "stats_sample";
    case OpKind::StatsFold:                  return "stats_fold";
    case OpKind::CheckpointBegin:            return "checkpoint_begin";
    case OpKind::CheckpointWriteHandleTable: return "checkpoint_write_handle_table";
    case OpKind::Pin:                        return "pin";
    case OpKind::Unpin:                      return "unpin";
    case OpKind::PinReleaseAll:              return "pin_release_all";
    case OpKind::DeferPush:                  return "defer_push";
    case OpKind::DeferCompact:               return "defer_compact";
    case OpKind::DeferFlush:                 return "defer_flush";
    case OpKind::PoolCompact:                return "pool_compact";
    case OpKind::ShardRebalance:             return "shard_rebalance";
    case OpKind::SpanGuardAttach:            return "span_guard_attach";
    case OpKind::SpanGuardVerify:            return "span_guard_verify";
    case OpKind::BatchStage:                 return "batch_stage";
    case OpKind::BatchCommit:                return "batch_commit";
    case OpKind::HubEvict:                   return "hub_evict";
    case OpKind::HubDestroy:                 return "hub_destroy";
    }
    return "unknown";
}

inline constexpr std::uint16_t op_kind_count() noexcept {
    return 33;
}

}  // namespace memforge

#endif  // MEMFORGE_OP_KINDS_HPP
