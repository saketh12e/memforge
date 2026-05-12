// Example: epoch progression demo.
//
// Drives the epoch through several tick/drain cycles and prints state
// after each.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    EpochQueue queue;
    queue.init();

    for (std::uint32_t cycle = 0; cycle < 3; ++cycle) {
        for (std::uint32_t i = 0; i < 4; ++i) {
            queue.enqueue(EpochEnqueueOp{cycle * 16 + i + 1, 16});
        }
        std::printf("cycle %u: pending=%zu\n", cycle, queue.pending());
        queue.tick(2);
        queue.drain(8);
        std::printf("cycle %u after drain: pending=%zu\n", cycle, queue.pending());
    }
    queue.shutdown();
    return 0;
}
