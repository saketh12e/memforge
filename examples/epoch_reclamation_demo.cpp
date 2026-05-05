// Example: epoch reclamation demo.
//
// Drives the EpochQueue through a tick/drain cycle and prints pending node
// counts before and after. Shows how a long-running server defers frees
// without blocking producers.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    EpochQueue queue;
    queue.init();
    for (std::uint32_t i = 0; i < 4; ++i) {
        queue.enqueue(EpochEnqueueOp{i + 1, 16});
    }
    std::printf("before tick: pending=%zu\n", queue.pending());
    queue.tick(2);
    queue.drain(8);
    std::printf("after drain: pending=%zu\n", queue.pending());

    // Run another cycle to demonstrate repeated reclamation.
    for (std::uint32_t i = 0; i < 4; ++i) {
        queue.enqueue(EpochEnqueueOp{100 + i, 32});
    }
    queue.tick(2);
    queue.drain(8);
    std::printf("after second drain: pending=%zu\n", queue.pending());
    queue.shutdown();
    return 0;
}
