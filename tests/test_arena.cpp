#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/arena_chain.hpp>

namespace {
void test_arena_basic() {
    using namespace memforge;
    ArenaChain chain;
    chain.init();
    for (std::uint32_t i = 0; i < 4; ++i) {
        chain.appendBlock(ArenaAppendOp{i + 1, 32, 8, i});
    }
    chain.finalizeBlocks();
    assert(chain.blockCount() == 4);
    assert(chain.finalizedBytes() >= 4u * 32u);
    chain.shutdown();
}

void test_arena_empty() {
    using namespace memforge;
    ArenaChain chain;
    chain.init();
    chain.finalizeBlocks();
    assert(chain.blockCount() == 0);
    assert(chain.finalizedBytes() == 0);
    chain.shutdown();
}

void test_arena_capacity() {
    using namespace memforge;
    ArenaChain chain;
    chain.init(2);
    assert(chain.appendBlock(ArenaAppendOp{1, 16, 8, 1}).ok());
    assert(chain.appendBlock(ArenaAppendOp{2, 16, 8, 2}).ok());
    assert(!chain.appendBlock(ArenaAppendOp{3, 16, 8, 3}).ok());
    chain.shutdown();
}

void test_arena_align_validation() {
    using namespace memforge;
    ArenaChain chain;
    chain.init();
    assert(!chain.appendBlock(ArenaAppendOp{1, 32, 7, 1}).ok());
    assert(chain.appendBlock(ArenaAppendOp{2, 32, 8, 2}).ok());
    chain.shutdown();
}

void test_arena_zero_bytes() {
    using namespace memforge;
    ArenaChain chain;
    chain.init();
    assert(!chain.appendBlock(ArenaAppendOp{1, 0, 8, 1}).ok());
    chain.shutdown();
}

void test_arena_apply_opstream() {
    using namespace memforge;
    OpStream ops;
    for (std::uint32_t i = 0; i < 8; ++i) {
        ops.appendArenaAppend(i + 1, 16, 8, i);
    }
    ArenaChain chain;
    chain.init();
    chain.apply(ops);
    assert(chain.blockCount() == 8);
    chain.shutdown();
}
}  // namespace

static memforge_test::Register r1("arena.basic", &test_arena_basic);
static memforge_test::Register r2("arena.empty", &test_arena_empty);
static memforge_test::Register r3("arena.capacity", &test_arena_capacity);
static memforge_test::Register r4("arena.align_validation", &test_arena_align_validation);
static memforge_test::Register r5("arena.zero_bytes", &test_arena_zero_bytes);
static memforge_test::Register r6("arena.apply_opstream", &test_arena_apply_opstream);
