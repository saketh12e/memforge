// MemForge - fuzz bridge.
//
// All harnesses share this bridge so the build script can compile them
// uniformly. The bridge decodes an op stream and drives the resource hub.

#include <cstddef>
#include <cstdint>

#include <memforge/api.hpp>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0) {
        return 0;
    }
    memforge::OpStream ops;
    ops.decode(memforge::ConstByteSpan{data, size});
    memforge::ResourceHub hub;
    hub.init();
    hub.applyOps(ops);
    hub.destroy();
    return 0;
}
