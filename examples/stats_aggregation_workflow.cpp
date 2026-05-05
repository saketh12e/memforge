// Example: stats aggregation workflow.
//
// Samples metrics across groups and folds them. Prints sample count and
// per-bucket fold outcomes.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    StatsAggregator agg;
    agg.init();
    for (std::uint32_t i = 0; i < 8; ++i) {
        agg.sample(StatsSampleOp{i + 1, (i % kStatsGroups) + 1, 4});
    }
    for (std::uint32_t b = 0; b < kStatsBuckets; ++b) {
        Status s = agg.foldMetrics(StatsFoldOp{b, 0});
        std::printf("fold bucket %u -> %s\n", b, s.ok() ? "ok" : "fail");
    }
    std::printf("samples: %zu\n", agg.sampleCount());
    agg.shutdown();
    return 0;
}
