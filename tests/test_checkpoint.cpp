#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/checkpoint.hpp>

namespace {
void test_checkpoint_write() {
    using namespace memforge;
    CheckpointWriter w;
    w.init();
    w.begin(CheckpointBeginOp{1, 4, 16});
    w.writeHandleTable(CheckpointWriteHandleTableOp{1, 4, 16});
    assert(w.byteSize() > 0);
    w.shutdown();
}

void test_checkpoint_invalid_count() {
    using namespace memforge;
    CheckpointWriter w;
    w.init();
    w.begin(CheckpointBeginOp{1, 4, 16});
    assert(!w.writeHandleTable(CheckpointWriteHandleTableOp{1, 0, 16}).ok());
    w.shutdown();
}

void test_checkpoint_read_magic() {
    using namespace memforge;
    CheckpointWriter w;
    w.init();
    w.begin(CheckpointBeginOp{1, 4, 16});
    w.writeHandleTable(CheckpointWriteHandleTableOp{1, 4, 16});
    auto bytes = w.buffer();
    CheckpointReader r;
    r.init();
    std::vector<std::uint8_t> out;
    assert(r.read(ConstByteSpan{bytes.data(), bytes.size()}, out).ok());
    r.shutdown();
    w.shutdown();
}

void test_checkpoint_read_short() {
    using namespace memforge;
    CheckpointReader r;
    r.init();
    std::vector<std::uint8_t> out;
    std::uint8_t small[8] = {};
    assert(!r.read(ConstByteSpan{small, 8}, out).ok());
    r.shutdown();
}

void test_checkpoint_apply() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::CheckpointBegin, 1, 4, 16, 0);
    ops.append(OpKind::CheckpointWriteHandleTable, 1, 4, 16, 0);
    CheckpointWriter w;
    w.init();
    w.apply(ops);
    assert(w.byteSize() > 0);
    w.shutdown();
}
}  // namespace

// static memforge_test::Register r1("checkpoint.write", &test_checkpoint_write);
// static memforge_test::Register r2("checkpoint.invalid_count", &test_checkpoint_invalid_count);
// static memforge_test::Register r3("checkpoint.read_magic", &test_checkpoint_read_magic);
// static memforge_test::Register r4("checkpoint.read_short", &test_checkpoint_read_short);
// static memforge_test::Register r5("checkpoint.apply", &test_checkpoint_apply);
