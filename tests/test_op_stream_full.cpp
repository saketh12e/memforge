#include <cassert>
#include <cstdio>
#include <fstream>

#include "test_framework.hpp"
#include <memforge/api.hpp>
#include <memforge/op_kinds.hpp>

// Op stream wire format integration tests.
namespace {
void test_opstream_roundtrip_all_kinds() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::ArenaAppendBlock, 1, 16, 8, 1);
    ops.append(OpKind::PoolAlloc, 0, 1, 0, 0);
    ops.append(OpKind::PoolFree, 0, 1, 0, 0);
    ops.append(OpKind::HandleInsert, 1, 1, 0, 0);
    ops.append(OpKind::HandleResolve, 1, 0, 0, 0);
    ops.append(OpKind::EpochEnqueue, 1, 16, 0, 0);
    ops.append(OpKind::EpochTick, 1, 0, 0, 0);
    ops.append(OpKind::EpochDrain, 1, 0, 0, 0);
    ops.append(OpKind::TxBegin, 1, 0, 0, 0);
    ops.append(OpKind::TxRecord, 1, 32, 0, 0);
    ops.append(OpKind::TxCommit, 1, 0, 0, 0);
    ops.append(OpKind::TxRollback, 1, 0, 0, 0);
    ops.append(OpKind::MigratePlan, 0, 0, 4, 32);
    ops.append(OpKind::MigrateCommit, 0, 0, 0, 0);
    ops.append(OpKind::StatsSample, 1, 1, 4, 0);
    ops.append(OpKind::StatsFold, 1, 1, 0, 0);
    ops.append(OpKind::CheckpointBegin, 1, 4, 16, 0);
    ops.append(OpKind::CheckpointWriteHandleTable, 1, 4, 16, 0);
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    ops.append(OpKind::PinReleaseAll, 0, 0, 0, 0);
    ops.append(OpKind::DeferPush, 1, 16, 0, 0);
    ops.append(OpKind::DeferCompact, 0, 0, 0, 0);
    ops.append(OpKind::DeferFlush, 0, 0, 0, 0);
    ops.append(OpKind::PoolCompact, 0, 0, 0, 0);
    ops.append(OpKind::ShardRebalance, 0, 1, 0, 0);
    ops.append(OpKind::SpanGuardAttach, 1, 2, 0, 0);
    ops.append(OpKind::SpanGuardVerify, 1, 0, 0, 0);
    ops.append(OpKind::BatchStage, 1, 8, 16, 0);
    ops.append(OpKind::BatchCommit, 1, 0, 0, 0);
    ops.append(OpKind::HubEvict, 1, 0, 0, 0);
    ops.append(OpKind::HubDestroy, 0, 0, 0, 0);

    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    assert(bytes.size() == 31 * 20);

    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded.size() == 31);
}

void test_opstream_partial_decode() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::ArenaAppendBlock, 1, 16, 8, 1);
    ops.append(OpKind::ArenaAppendBlock, 2, 32, 8, 2);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    // Truncate to one full record plus a few trailing bytes.
    assert(bytes.size() == 40);
    std::vector<std::uint8_t> trunc(bytes.begin(), bytes.begin() + 25);
    OpStream decoded;
    decoded.decode(ConstByteSpan{trunc.data(), trunc.size()});
    assert(decoded.size() == 1);
}

void test_op_kind_names() {
    using namespace memforge;
    assert(std::string(op_kind_name(OpKind::ArenaAppendBlock)) == "arena_append");
    assert(std::string(op_kind_name(OpKind::HubDestroy)) == "hub_destroy");
    assert(std::string(op_kind_name(OpKind::Noop)) == "noop");
    assert(op_kind_count() == 33);
}

void test_accessors_complete() {
    using namespace memforge;
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    ArenaAppendOp a = ops[0].asArenaAppend();
    assert(a.block_id == 1);
    assert(a.byte_count == 16);
    assert(a.align == 8);
    assert(a.payload_crc == 1);

    PoolAllocOp p = ops[0].asPoolAlloc();
    assert(p.class_id == 1);
}

void test_opstream_file_roundtrip() {
    using namespace memforge;
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);

    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);

    const std::string path = "/tmp/memforge_opstream_test.bin";
    {
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    std::ifstream in(path, std::ios::binary);
    std::vector<std::uint8_t> readback{std::istreambuf_iterator<char>(in),
                                       std::istreambuf_iterator<char>()};
    OpStream decoded;
    decoded.decode(ConstByteSpan{readback.data(), readback.size()});
    assert(decoded.size() == 2);
}
}  // namespace

static memforge_test::Register r1("opstream.all_kinds", &test_opstream_roundtrip_all_kinds);
static memforge_test::Register r2("opstream.partial", &test_opstream_partial_decode);
static memforge_test::Register r3("opstream.op_kind_names", &test_op_kind_names);
static memforge_test::Register r4("opstream.accessors_complete", &test_accessors_complete);
static memforge_test::Register r5("opstream.file_roundtrip", &test_opstream_file_roundtrip);
