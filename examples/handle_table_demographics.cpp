// Example: handle table demographics.
//
// Inserts handles into a single slot multiple times to bump the generation
// counter and prints the slot demographics after each bump.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    HandleTable table;
    table.init();

    std::uint32_t packed = 0;
    for (std::uint32_t gen = 1; gen <= 8; ++gen) {
        table.insert(HandleInsertOp{1, gen}, packed);
        std::printf("insert gen=%u packed=0x%08x\n", gen, packed);
    }
    std::printf("final size: %zu\n", table.size());
    table.shutdown();
    return 0;
}
