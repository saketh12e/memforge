#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/shard_map.hpp>
#include <memforge/slab_cache.hpp>

namespace {
void test_shard_rebalance() {
    using namespace memforge;
    ShardMap shards;
    SlabCache a;
    SlabCache b;
    a.init();
    b.init();
    shards.init();
    shards.attachShard(0, &a);
    shards.attachShard(1, &b);
    shards.rebalance(0, 1);
    shards.shutdown();
    a.shutdown();
    b.shutdown();
}

void test_shard_attach_invalid() {
    using namespace memforge;
    ShardMap shards;
    shards.init();
    SlabCache c;
    c.init();
    assert(!shards.attachShard(kNumShards, &c).ok());
    assert(!shards.attachShard(0, nullptr).ok());
    c.shutdown();
    shards.shutdown();
}

void test_shard_rebalance_same() {
    using namespace memforge;
    ShardMap shards;
    SlabCache a;
    a.init();
    shards.init();
    shards.attachShard(0, &a);
    assert(!shards.rebalance(0, 0).ok());
    a.shutdown();
    shards.shutdown();
}

void test_shard_rebalance_missing() {
    using namespace memforge;
    ShardMap shards;
    SlabCache a;
    a.init();
    shards.init();
    shards.attachShard(0, &a);
    assert(!shards.rebalance(0, 1).ok());
    a.shutdown();
    shards.shutdown();
}

void test_shard_apply() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::ShardRebalance, 0, 1, 0, 0);
    ShardMap shards;
    SlabCache a;
    SlabCache b;
    a.init();
    b.init();
    shards.init();
    shards.attachShard(0, &a);
    shards.attachShard(1, &b);
    shards.apply(ops);
    shards.shutdown();
    a.shutdown();
    b.shutdown();
}
}  // namespace

// static memforge_test::Register r1("shard.rebalance", &test_shard_rebalance);
// static memforge_test::Register r2("shard.attach_invalid", &test_shard_attach_invalid);
// static memforge_test::Register r3("shard.rebalance_same", &test_shard_rebalance_same);
// static memforge_test::Register r4("shard.rebalance_missing", &test_shard_rebalance_missing);
// static memforge_test::Register r5("shard.apply", &test_shard_apply);
