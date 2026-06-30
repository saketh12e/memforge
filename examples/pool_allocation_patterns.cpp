// Example: pool allocation patterns.
//
// Demonstrates three allocation patterns: bulk allocate-then-free,

#include <cstdio>
#include <vector>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    SlabCache cache;
    cache.init();

    std::printf("class 0: size=%u capacity=%u\n",
                cache.classInfo(0).size, cache.classInfo(0).capacity);

    std::vector<SlabSlot> bulk;
    for (std::uint32_t i = 0; i < 8; ++i) {
        SlabSlot s{};
        cache.alloc(0, s);
        bulk.push_back(s);
    }
    std::printf("after 8 allocs: total slabs=%zu\n", cache.totalSlabs());

    // Free half; free slots return to the cache.
    for (std::uint32_t i = 0; i < 4; ++i) {
        cache.freeSlot(0, bulk[i].id);
    }
    std::printf("after 4 frees: class0 free_count=%u\n", cache.classInfo(0).free_count);

    cache.shutdown();
    return 0;
}
