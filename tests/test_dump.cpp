#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/api.hpp>
#include <memforge/dump.hpp>
#include <memforge/rate_counter.hpp>
#include <memforge/hex.hpp>

namespace {
void test_dump_metrics() {
    using namespace memforge;
    HubMetrics m{};
    m.arena.blocks = 4;
    m.pool.slabs = 8;
    TextDumper d(stdout);
    d.dump_metrics(m);
}

void test_dump_op() {
    using namespace memforge;
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    TextDumper d(stdout);
    d.dump_op(ops[0], 0);
}

void test_rate_counter_record() {
    using namespace memforge;
    RateCounter c;
    c.record(100, 1000000);
    c.record(200, 2000000);
    c.record(300, 3000000);
    assert(c.count() == 3);
    assert(c.total() == 600);
}

void test_rate_counter_rate() {
    using namespace memforge;
    RateCounter c;
    c.record(1000, 0);
    c.record(1000, 1000000000);  // 1 second later
    double rate = c.rate_per_second(1000000000);
    assert(rate >= 0.0);
}

void test_rate_counter_reset() {
    using namespace memforge;
    RateCounter c;
    c.record(100, 0);
    c.reset();
    assert(c.count() == 0);
    assert(c.total() == 0);
}

void test_hex_dump_basic() {
    using namespace memforge;
    std::uint8_t bytes[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    HexDump::dump(stdout, bytes, 16);
}

void test_hex_dump_empty() {
    using namespace memforge;
    HexDump::dump(stdout, nullptr, 0);
    HexDump::dump(stdout, nullptr, 100);
}

void test_hex_dump_truncated() {
    using namespace memforge;
    std::uint8_t bytes[100] = {};
    for (int i = 0; i < 100; ++i) bytes[i] = static_cast<std::uint8_t>(i);
    HexDump::dump(stdout, bytes, 100, 32);
}

void test_hex_format_byte() {
    using namespace memforge;
    char buf[3];
    HexDump::format_byte(0xAB, buf);
    assert(buf[0] == 'a');
    assert(buf[1] == 'b');
    HexDump::format_byte(0x0F, buf);
    assert(buf[0] == '0');
    assert(buf[1] == 'f');
}
}  // namespace

static memforge_test::Register r1("dump.metrics", &test_dump_metrics);
static memforge_test::Register r2("dump.op", &test_dump_op);
static memforge_test::Register r3("rate.record", &test_rate_counter_record);
static memforge_test::Register r4("rate.rate", &test_rate_counter_rate);
static memforge_test::Register r5("rate.reset", &test_rate_counter_reset);
static memforge_test::Register r6("hex.basic", &test_hex_dump_basic);
static memforge_test::Register r7("hex.empty", &test_hex_dump_empty);
static memforge_test::Register r8("hex.truncated", &test_hex_dump_truncated);
static memforge_test::Register r9("hex.format_byte", &test_hex_format_byte);
