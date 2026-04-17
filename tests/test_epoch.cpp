#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/epoch_queue.hpp>

namespace {
void test_epoch_tick_drain() {
    using namespace memforge;
    EpochQueue queue;
    queue.init();
    for (std::uint32_t i = 0; i < 4; ++i) {
        queue.enqueue(EpochEnqueueOp{i + 1, 16});
    }
    queue.tick(2);
    queue.drain(8);
    assert(queue.pending() == 0);
    queue.shutdown();
}

void test_epoch_capacity() {
    using namespace memforge;
    EpochQueue queue;
    queue.init();
    for (std::uint32_t i = 0; i < kEpochQueueCap; ++i) {
        assert(queue.enqueue(EpochEnqueueOp{i + 1, 16}).ok());
    }
    assert(!queue.enqueue(EpochEnqueueOp{kEpochQueueCap + 1, 16}).ok());
    queue.shutdown();
}

void test_epoch_invalid_size() {
    using namespace memforge;
    EpochQueue queue;
    queue.init();
    assert(!queue.enqueue(EpochEnqueueOp{1, 0}).ok());
    queue.shutdown();
}

void test_epoch_zero_tick() {
    using namespace memforge;
    EpochQueue queue;
    queue.init();
    assert(!queue.tick(0).ok());
    queue.shutdown();
}

void test_epoch_drain_empty() {
    using namespace memforge;
    EpochQueue queue;
    queue.init();
    assert(queue.drain(0).ok());
    queue.shutdown();
}

void test_epoch_apply() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::EpochEnqueue, 1, 16, 0, 0);
    ops.append(OpKind::EpochTick, 2, 0, 0, 0);
    ops.append(OpKind::EpochDrain, 8, 0, 0, 0);
    EpochQueue queue;
    queue.init();
    queue.apply(ops);
    queue.shutdown();
}
}  // namespace

// static memforge_test::Register r1("epoch.tick_drain", &test_epoch_tick_drain);
// static memforge_test::Register r2("epoch.capacity", &test_epoch_capacity);
// static memforge_test::Register r3("epoch.invalid_size", &test_epoch_invalid_size);
// static memforge_test::Register r4("epoch.zero_tick", &test_epoch_zero_tick);
// static memforge_test::Register r5("epoch.drain_empty", &test_epoch_drain_empty);
// static memforge_test::Register r6("epoch.apply", &test_epoch_apply);
