#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/resource_hub.hpp>

namespace {
void test_hub_destroy() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    hub.applyOps(ops);
    hub.destroy();
}

void test_hub_double_destroy() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    hub.destroy();
    assert(hub.destroy().ok());
}

void test_hub_evict_missing() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    assert(!hub.evictBucket(999, false).ok());
    hub.destroy();
}

void test_hub_evict_keep_summary() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    assert(hub.evictBucket(1, true).ok());
    hub.destroy();
}

void test_hub_apply_ops() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    ops.append(OpKind::HandleInsert, 1, 1, 0, 0);
    ops.append(OpKind::StatsSample, 1, 1, 4, 0);
    ops.append(OpKind::BatchStage, 1, 8, 16, 0);
    ops.append(OpKind::BatchCommit, 1, 0, 0, 0);
    ops.append(OpKind::SpanGuardAttach, 1, 2, 0, 0);
    hub.applyOps(ops);
    assert(hub.bucketCount() == 8);
    hub.destroy();
}

void test_hub_populate_empty() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    hub.applyOps(ops);
    assert(hub.bucketCount() == 8);
    hub.destroy();
}
}  // namespace

// static memforge_test::Register r1("hub.destroy", &test_hub_destroy);
// static memforge_test::Register r2("hub.double_destroy", &test_hub_double_destroy);
// static memforge_test::Register r3("hub.evict_missing", &test_hub_evict_missing);
// static memforge_test::Register r4("hub.evict_keep_summary", &test_hub_evict_keep_summary);
// static memforge_test::Register r5("hub.apply_ops", &test_hub_apply_ops);
// static memforge_test::Register r6("hub.populate_empty", &test_hub_populate_empty);
