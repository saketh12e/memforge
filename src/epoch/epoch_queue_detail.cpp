// MemForge - epoch queue deep implementation.

#include <memforge/epoch_queue.hpp>
#include <memforge/align.hpp>

namespace memforge {

namespace epoch_detail {

std::uint32_t delta_to_retire(std::uint32_t current, std::uint32_t enqueued) noexcept {
    if (current < enqueued) {
        return 0;
    }
    return current - enqueued;
}

std::uint32_t ring_index(std::uint32_t epoch, std::uint32_t slot) noexcept {
    return (epoch * 2u + slot) & 1u;
}

std::uint32_t max_retired_capacity() noexcept {
    return 8192;
}

}  // namespace epoch_detail

std::uint32_t EpochQueue::deltaToRetire(std::uint32_t current,
                                        std::uint32_t enqueued) noexcept {
    return epoch_detail::delta_to_retire(current, enqueued);
}

std::uint32_t EpochQueue::retiredCapacityLimit() noexcept {
    return epoch_detail::max_retired_capacity();
}

}  // namespace memforge
