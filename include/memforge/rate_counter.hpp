// MemForge - rate counter with EWMA.
//
// A small rolling-window counter used by the stats aggregator for hot paths.

#ifndef MEMFORGE_RATE_COUNTER_HPP
#define MEMFORGE_RATE_COUNTER_HPP

#include <cstdint>

namespace memforge {

class RateCounter {
public:
    RateCounter() = default;
    ~RateCounter() = default;

    void record(std::uint64_t value, std::uint64_t now_ns);
    double rate_per_second(std::uint64_t window_ns) const noexcept;

    std::uint64_t total() const noexcept { return total_; }
    std::uint64_t count() const noexcept { return count_; }
    double ewma() const noexcept { return ewma_; }

    void reset() noexcept;

private:
    std::uint64_t total_{0};
    std::uint64_t count_{0};
    double ewma_{0.0};
    std::uint64_t last_ns_{0};
};

}  // namespace memforge

#endif  // MEMFORGE_RATE_COUNTER_HPP
