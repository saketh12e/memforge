// Example: shard rebalance workflow.
//
// Drives a ShardMap through attach/rebalance and prints the move count.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    SlabCache a;
    SlabCache b;
    a.init();
    b.init();
    for (std::uint32_t i = 0; i < 4; ++i) {
        SlabSlot s{};
        a.alloc(0, s);
        b.alloc(0, s);
    }
    ShardMap shards;
    shards.init();
    shards.attachShard(0, &a);
    shards.attachShard(1, &b);
    shards.rebalance(0, 1);
    std::printf("shard moves: %zu\n", shards.moves().size());
    shards.shutdown();
    a.shutdown();
    b.shutdown();
    return 0;
}
