#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/handle_table.hpp>

namespace {
void test_handle_insert_resolve() {
    using namespace memforge;
    HandleTable table;
    table.init();
    for (std::uint32_t i = 0; i < 4; ++i) {
        std::uint32_t handle = 0;
        table.insert(HandleInsertOp{i + 1, i + 1}, handle);
    }
    assert(table.size() == 4);
    table.shutdown();
}

void test_handle_invalid_slot() {
    using namespace memforge;
    HandleTable table;
    table.init();
    std::uint32_t handle = 0;
    assert(!table.insert(HandleInsertOp{0, 1}, handle).ok());
    assert(!table.insert(HandleInsertOp{kHandleTableSlots, 1}, handle).ok());
    table.shutdown();
}

void test_handle_resolve_missing() {
    using namespace memforge;
    HandleTable table;
    table.init();
    std::uint8_t* slot = nullptr;
    assert(!table.resolve(1, slot).ok());
    table.shutdown();
}

void test_handle_generation_bump() {
    using namespace memforge;
    HandleTable table;
    table.init();
    std::uint32_t h1 = 0;
    table.insert(HandleInsertOp{1, 1}, h1);
    std::uint32_t h2 = 0;
    table.insert(HandleInsertOp{1, 2}, h2);
    assert(h1 != h2);
    table.shutdown();
}

void test_handle_apply_opstream() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::HandleInsert, 1, 1, 0, 0);
    ops.append(OpKind::HandleInsert, 2, 1, 0, 0);
    ops.append(OpKind::HandleResolve, 1, 0, 0, 0);
    HandleTable table;
    table.init();
    table.apply(ops);
    assert(table.size() == 2);
    table.shutdown();
}
}  // namespace

static memforge_test::Register r1("handle.insert_resolve", &test_handle_insert_resolve);
static memforge_test::Register r2("handle.invalid_slot", &test_handle_invalid_slot);
static memforge_test::Register r3("handle.resolve_missing", &test_handle_resolve_missing);
static memforge_test::Register r4("handle.generation_bump", &test_handle_generation_bump);
static memforge_test::Register r5("handle.apply_opstream", &test_handle_apply_opstream);
