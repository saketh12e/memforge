#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/api.hpp>

// Tests for serializing and deserializing op streams via the public API.
// Verifies the wire format is stable across writes.

namespace {
void test_serialize_empty() {
    using namespace memforge;
    OpStream ops;
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    assert(bytes.empty());
}

void test_serialize_single() {
    using namespace memforge;
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    assert(bytes.size() == 20);
}

void test_serialize_many() {
    using namespace memforge;
    OpStream ops;
    for (std::uint32_t i = 0; i < 64; ++i) {
        ops.appendArenaAppend(i + 1, 32, 8, i);
    }
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    assert(bytes.size() == 64 * 20);
}

void test_serialize_then_deserialize() {
    using namespace memforge;
    OpStream ops;
    for (std::uint32_t i = 0; i < 16; ++i) {
        ops.append(OpKind::PoolAlloc, i % 6, 0, 0, 0);
    }
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);

    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded.size() == 16);
    for (std::size_t i = 0; i < 16; ++i) {
        assert(decoded[i].kind == OpKind::PoolAlloc);
        assert(decoded[i].a == (i % 6));
    }
}

void test_serialize_short_partial() {
    using namespace memforge;
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    ops.appendArenaAppend(2, 32, 8, 2);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    assert(bytes.size() == 40);
    std::vector<std::uint8_t> trunc(bytes.begin(), bytes.begin() + 20);
    OpStream decoded;
    decoded.decode(ConstByteSpan{trunc.data(), trunc.size()});
    assert(decoded.size() == 1);
}

void test_serialize_three_bytes() {
    using namespace memforge;
    std::vector<std::uint8_t> bytes = {1, 2, 3};
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded.size() == 0);
}

void test_serialize_zero_class_id() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded[0].a == 0);
}

void test_serialize_max_class_id() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::PoolAlloc, 0xFFFE, 0xFFFF, 0xFFFF, 0xFFFF);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded[0].a == 0xFFFE);
    assert(decoded[0].d == 0xFFFF);
}

void test_serialize_round_trip_pool() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    ops.append(OpKind::PoolAlloc, 1, 0, 0, 0);
    ops.append(OpKind::PoolFree, 0, 1, 0, 0);
    ops.append(OpKind::PoolCompact, 0, 0, 0, 0);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded.size() == 4);
    assert(decoded[0].kind == OpKind::PoolAlloc);
    assert(decoded[1].kind == OpKind::PoolAlloc);
    assert(decoded[2].kind == OpKind::PoolFree);
    assert(decoded[3].kind == OpKind::PoolCompact);
}

void test_serialize_round_trip_handle() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::HandleInsert, 1, 1, 0, 0);
    ops.append(OpKind::HandleInsert, 2, 2, 0, 0);
    ops.append(OpKind::HandleResolve, 1, 0, 0, 0);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded[0].kind == OpKind::HandleInsert);
    assert(decoded[1].kind == OpKind::HandleInsert);
    assert(decoded[2].kind == OpKind::HandleResolve);
}

void test_serialize_round_trip_epoch() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::EpochEnqueue, 1, 16, 0, 0);
    ops.append(OpKind::EpochTick, 2, 0, 0, 0);
    ops.append(OpKind::EpochDrain, 8, 0, 0, 0);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded[0].kind == OpKind::EpochEnqueue);
    assert(decoded[1].kind == OpKind::EpochTick);
    assert(decoded[2].kind == OpKind::EpochDrain);
}

void test_serialize_round_trip_stats() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::StatsSample, 1, 1, 4, 0);
    ops.append(OpKind::StatsFold, 1, 0, 0, 0);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded.size() == 2);
}

void test_serialize_round_trip_checkpoint() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::CheckpointBegin, 1, 4, 16, 0);
    ops.append(OpKind::CheckpointWriteHandleTable, 1, 4, 16, 0);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded.size() == 2);
    assert(decoded[1].kind == OpKind::CheckpointWriteHandleTable);
}

void test_serialize_round_trip_pin() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    ops.append(OpKind::Pin, 2, 1, 0, 0);
    ops.append(OpKind::PinReleaseAll, 0, 0, 0, 0);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded.size() == 3);
    assert(decoded[1].b == 1);
}

void test_serialize_round_trip_defer() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::DeferPush, 1, 16, 0, 0);
    ops.append(OpKind::DeferCompact, 0, 0, 0, 0);
    ops.append(OpKind::DeferFlush, 0, 0, 0, 0);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded.size() == 3);
}

void test_serialize_round_trip_batch() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::BatchStage, 1, 8, 16, 0);
    ops.append(OpKind::BatchCommit, 1, 0, 0, 0);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded.size() == 2);
    assert(decoded[0].b == 8);
    assert(decoded[0].c == 16);
}

void test_serialize_round_trip_guard() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::SpanGuardAttach, 1, 2, 0, 0);
    ops.append(OpKind::SpanGuardVerify, 1, 0, 0, 0);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded.size() == 2);
}

void test_serialize_round_trip_scope() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::TxBegin, 1, 0, 0, 0);
    ops.append(OpKind::TxRecord, 1, 32, 0, 0);
    ops.append(OpKind::TxCommit, 1, 0, 0, 0);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded.size() == 3);
}

void test_serialize_round_trip_migrate() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::MigratePlan, 0, 0, 4, 32);
    ops.append(OpKind::MigrateCommit, 0, 0, 0, 0);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded.size() == 2);
    assert(decoded[0].c == 4);
}

void test_serialize_round_trip_shard() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::ShardRebalance, 0, 1, 0, 0);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded[0].a == 0);
    assert(decoded[0].b == 1);
}
}  // namespace

static memforge_test::Register r1("serialize.empty", &test_serialize_empty);
static memforge_test::Register r2("serialize.single", &test_serialize_single);
static memforge_test::Register r3("serialize.many", &test_serialize_many);
static memforge_test::Register r4("serialize.round_trip", &test_serialize_then_deserialize);
static memforge_test::Register r5("serialize.short_partial", &test_serialize_short_partial);
static memforge_test::Register r6("serialize.three_bytes", &test_serialize_three_bytes);
static memforge_test::Register r7("serialize.zero_class_id", &test_serialize_zero_class_id);
static memforge_test::Register r8("serialize.max_class_id", &test_serialize_max_class_id);
static memforge_test::Register r9("serialize.pool_round_trip", &test_serialize_round_trip_pool);
static memforge_test::Register r10("serialize.handle_round_trip", &test_serialize_round_trip_handle);
static memforge_test::Register r11("serialize.epoch_round_trip", &test_serialize_round_trip_epoch);
static memforge_test::Register r12("serialize.stats_round_trip", &test_serialize_round_trip_stats);
static memforge_test::Register r13("serialize.checkpoint_round_trip", &test_serialize_round_trip_checkpoint);
static memforge_test::Register r14("serialize.pin_round_trip", &test_serialize_round_trip_pin);
static memforge_test::Register r15("serialize.defer_round_trip", &test_serialize_round_trip_defer);
static memforge_test::Register r16("serialize.batch_round_trip", &test_serialize_round_trip_batch);
static memforge_test::Register r17("serialize.guard_round_trip", &test_serialize_round_trip_guard);
static memforge_test::Register r18("serialize.scope_round_trip", &test_serialize_round_trip_scope);
static memforge_test::Register r19("serialize.migrate_round_trip", &test_serialize_round_trip_migrate);
static memforge_test::Register r20("serialize.shard_round_trip", &test_serialize_round_trip_shard);
