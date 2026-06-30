#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/api.hpp>
#include <memforge/profiler.hpp>
#include <memforge/settings.hpp>

namespace {
void test_profiler_basic() {
    using namespace memforge;
    MemoryProfiler p;
    HubMetrics m{};
    p.snapshot(m);
    assert(p.snapshot_count() == 1);
    p.clear();
    assert(p.snapshot_count() == 0);
}

void test_profiler_delta() {
    using namespace memforge;
    MemoryProfiler p;
    HubMetrics a{};
    a.arena.blocks = 1;
    a.pool.slabs = 2;
    HubMetrics b{};
    b.arena.blocks = 5;
    b.pool.slabs = 7;
    p.snapshot(a);
    p.snapshot(b);
    HubMetrics d = p.delta(0, 1);
    assert(d.arena.blocks == 4);
    assert(d.pool.slabs == 5);
}

void test_profiler_total() {
    using namespace memforge;
    MemoryProfiler p;
    HubMetrics m{};
    m.arena.blocks = 3;
    m.pool.slabs = 4;
    p.snapshot(m);
    p.snapshot(m);
    p.snapshot(m);
    HubMetrics t = p.total();
    assert(t.arena.blocks == 9);
    assert(t.pool.slabs == 12);
}

void test_settings_set_get() {
    using namespace memforge;
    Settings s;
    s.set("key1", "value1");
    assert(s.get("key1") == "value1");
    assert(s.get("missing", "default") == "default");
    assert(s.size() == 1);
}

void test_settings_bool() {
    using namespace memforge;
    Settings s;
    s.set("a", "true");
    s.set("b", "0");
    s.set("c", "yes");
    s.set("d", "invalid");
    assert(s.get_bool("a") == true);
    assert(s.get_bool("b") == false);
    assert(s.get_bool("c") == true);
    assert(s.get_bool("d", true) == true);
    assert(s.get_bool("missing", false) == false);
}

void test_settings_u32() {
    using namespace memforge;
    Settings s;
    s.set("count", "42");
    assert(s.get_u32("count") == 42);
    assert(s.get_u32("missing", 100) == 100);
    s.set("invalid", "abc");
    assert(s.get_u32("invalid", 7) == 7);
}
}  // namespace

static memforge_test::Register r1("profiler.basic", &test_profiler_basic);
static memforge_test::Register r2("profiler.delta", &test_profiler_delta);
static memforge_test::Register r3("profiler.total", &test_profiler_total);
static memforge_test::Register r4("settings.set_get", &test_settings_set_get);
static memforge_test::Register r5("settings.bool", &test_settings_bool);
static memforge_test::Register r6("settings.u32", &test_settings_u32);
