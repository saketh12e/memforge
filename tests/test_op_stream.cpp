#include <cassert>
#include <cstdio>
#include <vector>

#include "test_framework.hpp"
#include <memforge/op_stream.hpp>

namespace {
void test_op_stream_roundtrip() {
    using namespace memforge;
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    ops.appendArenaAppend(2, 32, 8, 2);
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    assert(bytes.size() == 40);
    OpStream decoded;
    decoded.decode(ConstByteSpan{bytes.data(), bytes.size()});
    assert(decoded.size() == 2);
    assert(decoded[0].kind == OpKind::ArenaAppendBlock);
}

void test_op_stream_empty() {
    using namespace memforge;
    OpStream ops;
    std::vector<std::uint8_t> bytes;
    ops.encode(bytes);
    assert(bytes.empty());
    OpStream decoded;
    assert(decoded.decode(ConstByteSpan{bytes.data(), bytes.size()}).ok());
    assert(decoded.size() == 0);
}

void test_op_stream_decode_short() {
    using namespace memforge;
    OpStream ops;
    std::uint8_t bytes[10] = {};
    ops.decode(ConstByteSpan{bytes, 10});
    assert(ops.size() == 0);
}

void test_op_stream_accessors() {
    using namespace memforge;
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    assert(!ops.empty());
    assert(ops.size() == 1);
    assert(ops[0].a == 1);
    ops.clear();
    assert(ops.empty());
}

void test_op_stream_iterators() {
    using namespace memforge;
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    ops.appendArenaAppend(2, 32, 8, 2);
    int count = 0;
    for (const auto& op : ops) {
        (void)op;
        ++count;
    }
    assert(count == 2);
}
}  // namespace

// static memforge_test::Register r1("opstream.roundtrip", &test_op_stream_roundtrip);
// static memforge_test::Register r2("opstream.empty", &test_op_stream_empty);
// static memforge_test::Register r3("opstream.decode_short", &test_op_stream_decode_short);
// static memforge_test::Register r4("opstream.accessors", &test_op_stream_accessors);
// static memforge_test::Register r5("opstream.iterators", &test_op_stream_iterators);
