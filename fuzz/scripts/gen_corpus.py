#!/usr/bin/env python3
# Generate per-harness seed corpora for MemForge.
#
# Each seed is a small, well-formed op stream that exercises the public
# surface of one module. The seeds are intentionally small so the fuzzer can
# mutate outward to reach deeper bugs.

import os
import struct
from pathlib import Path

OP_RECORD = struct.Struct("<HHIIII")
ROOT = Path(__file__).resolve().parent.parent / "corpus"

OP = {
    "noop": 0,
    "arena_append": 1,
    "pool_alloc": 2,
    "pool_free": 3,
    "handle_insert": 4,
    "handle_resolve": 5,
    "epoch_enqueue": 6,
    "epoch_tick": 7,
    "epoch_drain": 8,
    "tx_begin": 9,
    "tx_record": 10,
    "tx_commit": 11,
    "tx_rollback": 12,
    "migrate_plan": 13,
    "migrate_commit": 14,
    "stats_sample": 15,
    "stats_fold": 16,
    "checkpoint_begin": 17,
    "checkpoint_write_handle_table": 18,
    "pin": 19,
    "unpin": 20,
    "pin_release_all": 21,
    "defer_push": 22,
    "defer_compact": 23,
    "defer_flush": 24,
    "pool_compact": 25,
    "shard_rebalance": 26,
    "span_guard_attach": 27,
    "span_guard_verify": 28,
    "batch_stage": 29,
    "batch_commit": 30,
    "hub_evict": 31,
    "hub_destroy": 32,
}


def op(kind, a=0, b=0, c=0, d=0):
    return OP_RECORD.pack(OP[kind], 0, a, b, c, d)


def write(path, *ops):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as f:
        for o in ops:
            f.write(o)


def arena():
    out = ROOT / "arena_fuzzer"
    write(out / "basic_arena_ops.bin",
          op("arena_append", 1, 32, 8, 1),
          op("arena_append", 2, 64, 8, 2))


def pool():
    out = ROOT / "pool_fuzzer"
    write(out / "slab_alloc_ops.bin",
          op("pool_alloc", 0, 0, 0, 0),
          op("pool_alloc", 1, 0, 0, 0),
          op("pool_alloc", 2, 0, 0, 0))


def handle():
    out = ROOT / "handle_fuzzer"
    write(out / "handle_resolve_ops.bin",
          op("handle_insert", 1, 1, 0, 0),
          op("handle_resolve", 1, 0, 0, 0))


def epoch():
    out = ROOT / "epoch_fuzzer"
    write(out / "epoch_drain_ops.bin",
          op("epoch_enqueue", 1, 16, 0, 0),
          op("epoch_tick", 2, 0, 0, 0),
          op("epoch_drain", 8, 0, 0, 0))


def scope():
    out = ROOT / "scope_fuzzer"
    write(out / "tx_rollback_ops.bin",
          op("tx_begin", 1, 0, 0, 0),
          op("tx_record", 1, 32, 0, 0),
          op("tx_rollback", 1, 0, 0, 0))


def migrate():
    out = ROOT / "migrate_fuzzer"
    write(out / "pool_migrate_ops.bin",
          op("migrate_plan", 0, 0, 4, 32),
          op("migrate_commit", 0, 0, 0, 0))


def stats():
    out = ROOT / "stats_fuzzer"
    write(out / "stats_fold_ops.bin",
          op("stats_sample", 1, 1, 4, 0),
          op("stats_sample", 2, 2, 8, 0),
          op("stats_fold", 1, 1, 0, 0))


def checkpoint():
    out = ROOT / "checkpoint_fuzzer"
    write(out / "small_checkpoint.bin",
          op("checkpoint_begin", 1, 4, 16, 0),
          op("checkpoint_write_handle_table", 1, 4, 16, 0))


def pin():
    out = ROOT / "pin_fuzzer"
    write(out / "pin_release_ops.bin",
          op("pin", 1, 0, 0, 0),
          op("pin_release_all", 0, 0, 0, 0))


def defer_list():
    out = ROOT / "defer_fuzzer"
    write(out / "defer_flush_ops.bin",
          op("defer_push", 1, 16, 0, 0),
          op("defer_push", 2, 16, 0, 0),
          op("defer_compact", 0, 0, 0, 0),
          op("defer_flush", 0, 0, 0, 0))


def compact():
    out = ROOT / "compact_fuzzer"
    write(out / "slab_compact_ops.bin",
          op("pool_alloc", 0, 0, 0, 0),
          op("pool_alloc", 1, 0, 0, 0),
          op("pool_compact", 0, 0, 0, 0))


def shard():
    out = ROOT / "shard_fuzzer"
    write(out / "shard_rebalance_ops.bin",
          op("shard_rebalance", 0, 1, 0, 0))


def guard():
    out = ROOT / "guard_fuzzer"
    write(out / "span_guard_ops.bin",
          op("span_guard_attach", 1, 2, 0, 0),
          op("span_guard_verify", 1, 0, 0, 0))


def batch():
    out = ROOT / "batch_fuzzer"
    write(out / "batch_commit_ops.bin",
          op("batch_stage", 1, 8, 16, 0),
          op("batch_commit", 1, 0, 0, 0))


def teardown():
    out = ROOT / "teardown_fuzzer"
    write(out / "hub_teardown_ops.bin",
          op("hub_evict", 1, 1, 0, 0),
          op("hub_destroy", 0, 0, 0, 0))


if __name__ == "__main__":
    arena()
    pool()
    handle()
    epoch()
    scope()
    migrate()
    stats()
    checkpoint()
    pin()
    defer_list()
    compact()
    shard()
    guard()
    batch()
    teardown()
    print("Generated seed corpora in", ROOT)
