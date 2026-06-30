// MemForge - arena harness.
//
// Decodes an op stream and runs it through the ArenaReplay workflow,
// which drives the arena finalize + checkpoint section materialization
// path.

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
    memforge::ArenaReplay replay;
    (void)replay.runFinalize(hub, ops);
    return 0;
}