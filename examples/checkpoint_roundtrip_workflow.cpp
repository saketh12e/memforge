// Example: checkpoint roundtrip workflow.
//
// Writes an op stream through the hub, builds a checkpoint, then reads it
// back. Demonstrates the binary checkpoint format end-to-end.

#include <cstdio>
#include <vector>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();

    OpStream ops;
    ops.append(OpKind::CheckpointBegin, 1, 4, 16, 0);
    ops.append(OpKind::CheckpointWriteHandleTable, 1, 4, 16, 0);
    hub.applyOps(ops);

    auto bytes = hub.writer().buffer();
    std::printf("checkpoint size: %zu bytes\n", bytes.size());

    CheckpointReader reader;
    reader.init();
    std::vector<std::uint8_t> out;
    Status s = reader.read(ConstByteSpan{bytes.data(), bytes.size()}, out);
    if (s.ok()) {
        std::printf("readback ok: %zu bytes\n", out.size());
    } else {
        std::printf("readback failed\n");
    }
    reader.shutdown();
    hub.destroy();
    return 0;
}
