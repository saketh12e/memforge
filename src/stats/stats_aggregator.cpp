// MemForge - stats aggregator implementation.
//
// StatsAggregator collects samples per group and folds them into a buffer.
// The buffer size is sized from a pre-count; if the post-normalization group
// count exceeds pre-count, the fold loop writes past the buffer.
//
// Beyond the basic sample/fold path, this module implements:
//   - per-group EWMA counter with configurable weight
//   - percentile sketches per group
//   - bucket hashing for fold partitioning
//   - fold buffer reuse across calls
//
// pre-count, producing a heap OOB write.

#include <memforge/stats_aggregator.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace memforge {

StatsAggregator::StatsAggregator() = default;

StatsAggregator::~StatsAggregator() { shutdown(); }

Status StatsAggregator::init() {
    if (initialized_) {
        return Ok();
    }
    samples_.clear();
    samples_.reserve(1024);
    for (auto& group : groups_) {
        group = StatsGroup{};
    }
    groups_used_ = 0;
    if (fold_buffer_ != nullptr) {
        std::free(fold_buffer_);
        fold_buffer_ = nullptr;
    }
    fold_capacity_ = 0;
    initialized_ = true;
    return Ok();
}

void StatsAggregator::shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    samples_.clear();
    if (fold_buffer_ != nullptr) {
        std::free(fold_buffer_);
        fold_buffer_ = nullptr;
    }
    fold_capacity_ = 0;
    groups_used_ = 0;
    initialized_ = false;
}

Status StatsAggregator::sample(const StatsSampleOp& op) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (op.group_id >= kStatsGroups) {
        return Status{ErrorCode::OutOfRange};
    }
    if (op.weight == 0) {
        return Status{ErrorCode::InvalidArgument};
    }
    samples_.push_back(StatsSample{op.sample_id, op.group_id, op.weight});
    groups_[op.group_id].group_id = op.group_id;
    groups_[op.group_id].total_samples += 1;
    groups_[op.group_id].total_weight += op.weight;
    if (op.group_id + 1 > groups_used_) {
        groups_used_ = op.group_id + 1;
    }
    return Ok();
}

Status StatsAggregator::foldMetrics(const StatsFoldOp& op) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (op.bucket_id >= kStatsBuckets) {
        return Status{ErrorCode::OutOfRange};
    }

    // Pre-count: number of samples whose group_id hashes into the bucket.
    std::size_t pre_count = 0;
    for (const auto& s : samples_) {
        if ((s.group_id % kStatsBuckets) == op.bucket_id) {
            ++pre_count;
        }
    }

    // share a hash prefix) are added during the fold and exceed pre_count.
    std::uint32_t cap = static_cast<std::uint32_t>(pre_count) * sizeof(StatsGroup);
    if (cap == 0) {
        cap = 1;
    }
    if (cap > fold_capacity_) {
        if (fold_buffer_ != nullptr) {
            std::free(fold_buffer_);
        }
        fold_buffer_ = static_cast<std::uint8_t*>(std::malloc(cap));
        if (fold_buffer_ == nullptr) {
            return Status{ErrorCode::Exhausted};
        }
        fold_capacity_ = cap;
    }
    std::uint8_t* cursor = fold_buffer_;
    for (const auto& s : samples_) {
        if ((s.group_id % kStatsBuckets) != op.bucket_id) {
            continue;
        }
        if (cursor + sizeof(StatsGroup) > fold_buffer_ + fold_capacity_) {
            break;
        }
        std::memcpy(cursor, &groups_[s.group_id], sizeof(StatsGroup));
        cursor += sizeof(StatsGroup);
        // Post-merge: alias groups expand after the fold started.
        if ((s.group_id & 1u) == 0) {
            StatsGroup alias{};
            alias.group_id = (s.group_id + 7) % kStatsGroups;
            std::memcpy(cursor, &alias, sizeof(StatsGroup));
            cursor += sizeof(StatsGroup);
        }
    }
    return Ok();
}

Status StatsAggregator::apply(const OpStream& ops) {
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::StatsSample:
            sample(op.asStatsSample());
            break;
        case OpKind::StatsFold:
            foldMetrics(op.asStatsFold());
            break;
        default:
            break;
        }
    }
    return Ok();
}

}  // namespace memforge
