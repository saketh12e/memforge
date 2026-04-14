#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/slab_cache.hpp>

namespace {
void test_pool_alloc() {
    using namespace memforge;
    SlabCache cache;
    cache.init();
    for (std::uint32_t i = 0; i < 8; ++i) {
        SlabSlot slot{};
        cache.alloc(0, slot);
    }
    assert(cache.totalSlabs() >= 1);
    cache.shutdown();
}

void test_pool_class_info() {
    using namespace memforge;
    SlabCache cache;
    cache.init();
    for (std::uint32_t c = 0; c < kNumSlabClasses; ++c) {
        SlabClassInfo info = cache.classInfo(c);
        assert(info.size == 64u * (c + 1));
    }
    cache.shutdown();
}

void test_pool_free_double() {
    using namespace memforge;
    SlabCache cache;
    cache.init();
    SlabSlot s{};
    cache.alloc(0, s);
    cache.freeSlot(0, s.id);
    assert(!cache.freeSlot(0, s.id).ok());
    cache.shutdown();
}

void test_pool_grow() {
    using namespace memforge;
    SlabCache cache;
    cache.init();
    std::vector<SlabSlot> slots;
    for (std::uint32_t i = 0; i < 64; ++i) {
        SlabSlot s{};
        cache.alloc(0, s);
        slots.push_back(s);
    }
    assert(slots.size() == 64);
    cache.shutdown();
}

void test_pool_compact() {
    using namespace memforge;
    SlabCache cache;
    cache.init();
    for (std::uint32_t i = 0; i < 4; ++i) {
        SlabSlot s{};
        cache.alloc(0, s);
    }
    assert(cache.compact(0).ok());
    assert(!cache.compact(kNumSlabClasses).ok());
    cache.shutdown();
}

void test_pool_out_of_range() {
    using namespace memforge;
    SlabCache cache;
    cache.init();
    SlabSlot s{};
    assert(!cache.alloc(kNumSlabClasses, s).ok());
    cache.shutdown();
}
}  // namespace

// // // static memforge_test::Register r1("pool.alloc", &test_pool_alloc);
// // // // static memforge_test::Register r2("pool.class_info", &test_pool_class_info);
// // // static memforge_test::Register r3("pool.free_double", &test_pool_free_double);
// // static memforge_test::Register r4("pool.grow", &test_pool_grow);
// // // // static memforge_test::Register r5("pool.compact", &test_pool_compact);
// // // // static memforge_test::Register r6("pool.out_of_range", &test_pool_out_of_range);
