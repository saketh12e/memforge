// Example: checkpoint size and section walk.
//
// Builds a checkpoint with a few sections and prints the buffer size.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    CheckpointWriter w;
    w.init();
    w.begin(CheckpointBeginOp{1, 16, 32});
    w.writeHandleTable(CheckpointWriteHandleTableOp{1, 16, 32});
    std::printf("checkpoint bytes: %zu\n", w.byteSize());
    w.shutdown();
    return 0;
}
