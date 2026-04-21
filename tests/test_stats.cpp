#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/stats_aggregator.hpp>

namespace {
void test_stats_sample_fold() {
    using namespace memforge;
    StatsAggregator agg;
    agg.init();
    for (std::uint32_t i = 0; i < 4; ++i) {
        agg.sample(StatsSampleOp{i + 1, i + 1, 8});
    }
    agg.foldMetrics(StatsFoldOp{1, 1});
    assert(agg.sampleCount() == 4);
    agg.shutdown();
}

void test_stats_invalid_group() {
    using namespace memforge;
    StatsAggregator agg;
    agg.init();
    assert(!agg.sample(StatsSampleOp{1, kStatsGroups, 8}).ok());
    agg.shutdown();
}

void test_stats_zero_weight() {
    using namespace memforge;
    StatsAggregator agg;
    agg.init();
    assert(!agg.sample(StatsSampleOp{1, 1, 0}).ok());
    agg.shutdown();
}

void test_stats_invalid_bucket() {
    using namespace memforge;
    StatsAggregator agg;
    agg.init();
    assert(!agg.foldMetrics(StatsFoldOp{kStatsBuckets, 0}).ok());
    agg.shutdown();
}

void test_stats_apply() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::StatsSample, 1, 1, 4, 0);
    ops.append(OpKind::StatsSample, 2, 2, 4, 0);
    ops.append(OpKind::StatsFold, 1, 0, 0, 0);
    StatsAggregator agg;
    agg.init();
    agg.apply(ops);
    assert(agg.sampleCount() == 2);
    agg.shutdown();
}
}  // namespace

// static memforge_test::Register r1("stats.sample_fold", &test_stats_sample_fold);
// static memforge_test::Register r2("stats.invalid_group", &test_stats_invalid_group);
// static memforge_test::Register r3("stats.zero_weight", &test_stats_zero_weight);
// static memforge_test::Register r4("stats.invalid_bucket", &test_stats_invalid_bucket);
// static memforge_test::Register r5("stats.apply", &test_stats_apply);
