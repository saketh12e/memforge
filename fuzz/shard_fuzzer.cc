// MemForge - shard harness.
//
// Decodes an op stream and runs it through the ShardReplay workflow.

#include <cstddef>
#include <cstdint>

#include <memforge/api.hpp>
#include <memforge/replay.hpp>
#include <memforge/replay/replay_workflows.hpp>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0) {
        return 0;
    }
    memforge::OpStream ops;
    if (!memforge::decodeAndValidate(data, size, ops)) {
        return 0;
    }
    memforge::ResourceHub hub;
    if (!hub.init()) {
        return 0;
    }
    memforge::SlabCache a;
    memforge::SlabCache b;
    memforge::ShardMap shards;
    if (!a.init() || !b.init() || !shards.init()) {
        return 0;
    }
    memforge::ShardReplay replay;
    (void)replay.runRebalance(hub, a, b, shards, ops);
    return 0;
}