// MemForge - scope harness.
//
// Decodes an op stream and runs it through the ScopeReplay workflow.

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
    memforge::ScopeReplay replay;
    (void)replay.runRollbackTeardown(hub, ops);
    return 0;
}