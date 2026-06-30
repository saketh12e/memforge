// MemForge - rate counter implementation.

#include <memforge/rate_counter.hpp>

namespace memforge {

void RateCounter::record(std::uint64_t value, std::uint64_t now_ns) {
    total_ += value;
    ++count_;
    if (count_ == 1) {
        ewma_ = static_cast<double>(value);
    } else {
        double alpha = 0.1;
        ewma_ = alpha * static_cast<double>(value) + (1.0 - alpha) * ewma_;
    }
    last_ns_ = now_ns;
}

double RateCounter::rate_per_second(std::uint64_t window_ns) const noexcept {
    if (window_ns == 0 || count_ == 0) {
        return 0.0;
    }
    double seconds = static_cast<double>(window_ns) / 1e9;
    if (seconds <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(total_) / seconds;
}

void RateCounter::reset() noexcept {
    total_ = 0;
    count_ = 0;
    ewma_ = 0.0;
    last_ns_ = 0;
}

}  // namespace memforge
