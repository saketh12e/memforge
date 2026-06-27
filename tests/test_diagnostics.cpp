#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/api.hpp>

// Diagnostic helpers and metrics collection tests.
namespace {
void test_arena_fragmented_bytes() {
    using namespace memforge;
    ArenaChain chain;
    chain.init();
    chain.appendBlock(ArenaAppendOp{1, 17, 8, 1});  // 7 bytes of alignment waste
    chain.appendBlock(ArenaAppendOp{2, 32, 8, 2});
    chain.appendBlock(ArenaAppendOp{3, 31, 16, 3});  // 1 byte of alignment waste
    chain.finalizeBlocks();
    std::uint32_t frag = chain.fragmentedBytes();
    assert(frag >= 8u);
    chain.shutdown();
}

void test_arena_header_trailer() {
    using namespace memforge;
    assert(ArenaChain::blockHeaderBytes() == 24);
    assert(ArenaChain::blockTrailerBytes() == 16);
}

void test_pool_fragmentation_ratio() {
    using namespace memforge;
    SlabCache cache;
    cache.init();
    for (std::uint32_t i = 0; i < 4; ++i) {
        SlabSlot s{};
        cache.alloc(0, s);
    }
    double ratio = cache.fragmentationRatio();
    assert(ratio >= 0.0);
    assert(ratio <= 1.0);
    cache.shutdown();
}

void test_handle_packing() {
    using namespace memforge;
    std::uint32_t packed = HandleTable::packedHandle(42, 7);
    assert(HandleTable::slotFromPacked(packed) == 42);
    assert(HandleTable::generationFromPacked(packed) == 7);
}

void test_epoch_delta() {
    using namespace memforge;
    assert(EpochQueue::deltaToRetire(10, 5) == 5);
    assert(EpochQueue::deltaToRetire(5, 10) == 0);
}

void test_scope_records_limit() {
    using namespace memforge;
    assert(TxScope::recordsPerScopeLimit() == 256);
}

void test_migrate_overhead() {
    using namespace memforge;
    assert(PoolMigrator::planHeaderBytes() == 32);
    assert(PoolMigrator::perSlotOverhead() == 16);
}

void test_pin_alias_limit() {
    using namespace memforge;
    assert(PinRegistry::aliasLimit() == 256);
}

void test_stats_ewma() {
    using namespace memforge;
    assert(StatsAggregator::ewmaUpdate(0, 100, 1) == 100);
    std::uint32_t v = StatsAggregator::ewmaUpdate(0, 100, 4);
    assert(v > 0);
}

void test_stats_bucket() {
    using namespace memforge;
    assert(StatsAggregator::bucketForGroup(0) == 0);
    assert(StatsAggregator::bucketForGroup(15) == 15);
}

void test_checkpoint_header_size() {
    using namespace memforge;
    assert(CheckpointWriter::headerSize() == 64);
    assert(CheckpointWriter::sectionOverhead() == 16);
}

void test_checkpoint_reader_magic() {
    using namespace memforge;
    std::uint8_t bytes[64] = {};
    std::memcpy(bytes, &magic, sizeof(magic));
    assert(CheckpointReader::verifyMagic(bytes));
    bytes[0] = 0;
    assert(!CheckpointReader::verifyMagic(bytes));
}

void test_hub_collect_metrics() {
    using namespace memforge;
    ResourceHub hub;
    hub.init();
    OpStream ops;
    ops.appendArenaAppend(1, 16, 8, 1);
    ops.append(OpKind::PoolAlloc, 0, 0, 0, 0);
    ops.append(OpKind::HandleInsert, 1, 1, 0, 0);
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    hub.applyOps(ops);
    HubMetrics m = hub.collectMetrics();
    assert(m.arena.blocks == 1);
    assert(m.buckets == 8);
    hub.destroy();
}
}  // namespace

static memforge_test::Register r1("diag.arena_fragmented", &test_arena_fragmented_bytes);
static memforge_test::Register r2("diag.arena_header_trailer", &test_arena_header_trailer);
static memforge_test::Register r3("diag.pool_fragmentation", &test_pool_fragmentation_ratio);
static memforge_test::Register r4("diag.handle_packing", &test_handle_packing);
static memforge_test::Register r5("diag.epoch_delta", &test_epoch_delta);
static memforge_test::Register r6("diag.scope_records_limit", &test_scope_records_limit);
static memforge_test::Register r7("diag.migrate_overhead", &test_migrate_overhead);
static memforge_test::Register r8("diag.pin_alias_limit", &test_pin_alias_limit);
static memforge_test::Register r9("diag.stats_ewma", &test_stats_ewma);
static memforge_test::Register r10("diag.stats_bucket", &test_stats_bucket);
static memforge_test::Register r11("diag.checkpoint_header", &test_checkpoint_header_size);
static memforge_test::Register r12("diag.checkpoint_reader_magic", &test_checkpoint_reader_magic);
static memforge_test::Register r13("diag.hub_collect_metrics", &test_hub_collect_metrics);
