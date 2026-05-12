// Example: deferred free list cycle.
//
// Pushes several deferred nodes, compacts, then flushes; prints the size
// at each step.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    DeferredFreeList list;
    list.init();

    for (std::uint32_t i = 0; i < 4; ++i) {
        list.push(DeferPushOp{i + 1, 32});
    }
    std::printf("after push: size=%zu\n", list.size());
    list.compact();
    std::printf("after compact: size=%zu\n", list.size());
    list.flush();
    std::printf("after flush: size=%zu\n", list.size());

    list.shutdown();
    return 0;
}
