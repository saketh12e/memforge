// Example: handle table churn.
//
// Inserts and resolves many handles to demonstrate generation bumps and
// safe resolution under churn. Prints handle count and resolution outcome.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    HandleTable table;
    table.init();

    std::uint32_t handles[8] = {};
    for (std::uint32_t i = 0; i < 8; ++i) {
        table.insert(HandleInsertOp{i + 1, i + 1}, handles[i]);
    }
    std::printf("handles inserted: %zu\n", table.size());

    std::uint8_t* slot = nullptr;
    for (std::uint32_t i = 0; i < 8; ++i) {
        Status s = table.resolve(handles[i], slot);
        std::printf("resolve %u -> %s\n", handles[i], s.ok() ? "ok" : "fail");
    }

    // Re-insert slot 1, generation bumps; old handle now stale.
    table.insert(HandleInsertOp{1, 99}, handles[0]);
    Status s = table.resolve(handles[0], slot);
    std::printf("after re-insert resolve -> %s\n", s.ok() ? "ok" : "stale");

    table.shutdown();
    return 0;
}
