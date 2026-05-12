// Example: stats fold workflow.
//
// Samples several groups and folds them across all buckets; prints per-bucket
// fold outcomes.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    StatsAggregator agg;
    agg.init();
    for (std::uint32_t i = 0; i < 16; ++i) {
        agg.sample(StatsSampleOp{i + 1, i + 1, 8});
    }
    int folds_ok = 0;
    for (std::uint32_t b = 0; b < kStatsBuckets; ++b) {
        if (agg.foldMetrics(StatsFoldOp{b, 0}).ok()) {
            ++folds_ok;
        }
    }
    std::printf("folds ok: %d/%u\n", folds_ok, kStatsBuckets);
    std::printf("samples: %zu groups: %zu\n", agg.sampleCount(), agg.groupCount());
    agg.shutdown();
    return 0;
}
