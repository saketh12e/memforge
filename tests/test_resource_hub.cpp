#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/resource_hub.hpp>

namespace {
void test_hub_lifecycle() {
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

void test_hub_module_access() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    (void)hub.arena();
    (void)hub.pool();
    (void)hub.handles();
    (void)hub.epoch();
    (void)hub.deferred();
    (void)hub.scope();
    (void)hub.migrator();
    (void)hub.pins();
    (void)hub.stats();
    (void)hub.writer();
    (void)hub.guards();
    (void)hub.batches();
    (void)hub.shards();
    hub.destroy();
}

void test_hub_evict_lifecycle() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    assert(hub.evictBucket(1, false).ok());
    assert(hub.evictBucket(2, true).ok());
    hub.destroy();
}

void test_hub_apply_then_evict() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.appendArenaAppend(1, 32, 8, 1);
    hub.applyOps(ops);
    assert(hub.evictBucket(1, false).ok());
    hub.destroy();
}
}  // namespace

// static memforge_test::Register r1("hub.lifecycle", &test_hub_lifecycle);
// static memforge_test::Register r2("hub.module_access", &test_hub_module_access);
// static memforge_test::Register r3("hub.evict_lifecycle", &test_hub_evict_lifecycle);
// static memforge_test::Register r4("hub.apply_then_evict", &test_hub_apply_then_evict);
