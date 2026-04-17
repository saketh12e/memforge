#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/deferred_free_list.hpp>

namespace {
void test_defer_push_flush() {
    using namespace memforge;
    DeferredFreeList list;
    list.init();
    list.push(DeferPushOp{1, 16});
    list.push(DeferPushOp{2, 16});
    list.compact();
    list.flush();
    assert(list.size() == 0);
    list.shutdown();
}

void test_defer_capacity() {
    using namespace memforge;
    DeferredFreeList list;
    list.init();
    for (std::uint32_t i = 0; i < kDeferListCap; ++i) {
        assert(list.push(DeferPushOp{i + 1, 16}).ok());
    }
    assert(!list.push(DeferPushOp{kDeferListCap + 1, 16}).ok());
    list.shutdown();
}

void test_defer_invalid_size() {
    using namespace memforge;
    DeferredFreeList list;
    list.init();
    assert(!list.push(DeferPushOp{1, 0}).ok());
    list.shutdown();
}

void test_defer_compact_empty() {
    using namespace memforge;
    DeferredFreeList list;
    list.init();
    assert(list.compact().ok());
    list.shutdown();
}

void test_defer_flush_empty() {
    using namespace memforge;
    DeferredFreeList list;
    list.init();
    assert(list.flush().ok());
    list.shutdown();
}

void test_defer_apply() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::DeferPush, 1, 16, 0, 0);
    ops.append(OpKind::DeferPush, 2, 16, 0, 0);
    ops.append(OpKind::DeferCompact, 0, 0, 0, 0);
    ops.append(OpKind::DeferFlush, 0, 0, 0, 0);
    DeferredFreeList list;
    list.init();
    list.apply(ops);
    list.shutdown();
}
}  // namespace

// static memforge_test::Register r1("defer.push_flush", &test_defer_push_flush);
// static memforge_test::Register r2("defer.capacity", &test_defer_capacity);
// static memforge_test::Register r3("defer.invalid_size", &test_defer_invalid_size);
// static memforge_test::Register r4("defer.compact_empty", &test_defer_compact_empty);
// static memforge_test::Register r5("defer.flush_empty", &test_defer_flush_empty);
// static memforge_test::Register r6("defer.apply", &test_defer_apply);
