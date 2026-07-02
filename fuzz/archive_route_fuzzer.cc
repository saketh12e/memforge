// MemForge - archive route harness.

#include <cstddef>
#include <cstdint>

#include <memforge/api.hpp>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0) {
        return 0;
    }
    memforge::ArchiveSession session;
    if (!session.load(memforge::ConstByteSpan{data, size})) {
        return 0;
    }
    (void)session.materializeRoutes();
    return 0;
}
