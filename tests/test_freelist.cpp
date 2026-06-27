#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/api.hpp>
#include <memforge/free_list.hpp>

// Tests for the FreeListAllocator utility.

namespace {
void test_freelist_basic() {
    using namespace memforge;
    FreeListAllocator a;
    assert(a.init(8, 32).ok());
    assert(a.free_count() == 8);
    std::uint8_t* s1 = nullptr;
    std::uint8_t* s2 = nullptr;
    assert(a.alloc(s1).ok());
    assert(a.alloc(s2).ok());
    assert(s1 != nullptr);
    assert(s2 != nullptr);
    assert(a.free_count() == 6);
    a.shutdown();
}

void test_freelist_exhaust() {
    using namespace memforge;
    FreeListAllocator a;
    assert(a.init(4, 32).ok());
    std::uint8_t* p = nullptr;
    for (int i = 0; i < 4; ++i) {
        assert(a.alloc(p).ok());
    }
    assert(!a.alloc(p).ok());
    a.shutdown();
}

void test_freelist_free() {
    using namespace memforge;
    FreeListAllocator a;
    assert(a.init(4, 32).ok());
    std::uint8_t* s = nullptr;
    assert(a.alloc(s).ok());
    assert(a.free_slot(s).ok());
    assert(a.free_count() == 4);
    a.shutdown();
}

void test_freelist_invalid_init() {
    using namespace memforge;
    FreeListAllocator a;
    assert(!a.init(0, 32).ok());
    assert(!a.init(4, 0).ok());
}

void test_freelist_double_init() {
    using namespace memforge;
    FreeListAllocator a;
    assert(a.init(4, 32).ok());
    assert(a.init(8, 32).ok());
    assert(a.free_count() == 8);
    a.shutdown();
}
}  // namespace

static memforge_test::Register r1("freelist.basic", &test_freelist_basic);
static memforge_test::Register r2("freelist.exhaust", &test_freelist_exhaust);
static memforge_test::Register r3("freelist.free", &test_freelist_free);
static memforge_test::Register r4("freelist.invalid_init", &test_freelist_invalid_init);
static memforge_test::Register r5("freelist.double_init", &test_freelist_double_init);
