// MemForge - replay entry-point implementation.

#include <memforge/replay.hpp>

#include <cstdint>

namespace memforge {

bool decodeAndValidate(const std::uint8_t* data, std::size_t size,
                       OpStream& out) {
    if (data == nullptr || size == 0) {
        return false;
    }
    out.clear();
    out.decode(ConstByteSpan{data, size});
    if (out.empty()) {
        return false;
    }
    return true;
}

Status applyToHub(ResourceHub& hub, const OpStream& ops) {
    return hub.applyOps(ops);
}

}  // namespace memforge