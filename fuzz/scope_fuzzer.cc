// MemForge - scope harness.
//
// Drives TxScope::record() and TxScope::rollback().

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
