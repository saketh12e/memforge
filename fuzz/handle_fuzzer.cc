// MemForge - handle harness.
//
// Decodes an op stream and runs it through the HandleReplay workflow.

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
    memforge::HandleReplay replay;
    (void)replay.runResolveWithGuard(hub, ops);
    return 0;
}