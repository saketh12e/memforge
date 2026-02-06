// MemForge - epoch-based deferred reclamation queue.

#ifndef MEMFORGE_EPOCH_QUEUE_HPP
#define MEMFORGE_EPOCH_QUEUE_HPP

#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

inline constexpr std::uint32_t kEpochQueueCap = 2048;
inline constexpr std::uint32_t kMaxEpochRetired = 8192;

struct EpochNode {
    std::uint32_t id{0};
    std::uint32_t epoch{0};
    std::uint32_t size_hint{0};
    std::uint8_t* payload{nullptr};
};

class EpochQueue {
public:
    EpochQueue();
    ~EpochQueue();

    EpochQueue(const EpochQueue&) = delete;
    EpochQueue& operator=(const EpochQueue&) = delete;

    Status init();
    void shutdown() noexcept;

    Status enqueue(const EpochEnqueueOp& op);
    Status tick(std::uint32_t advance);
    Status drain(std::uint32_t cap);

    std::uint32_t epoch() const noexcept { return current_epoch_; }
    std::size_t pending() const noexcept { return queue_.size(); }

    static std::uint32_t deltaToRetire(std::uint32_t current,
                                       std::uint32_t enqueued) noexcept;
    static std::uint32_t retiredCapacityLimit() noexcept;

    Status apply(const OpStream& ops);

private:
    void retire(std::vector<EpochNode>::iterator it);

    std::vector<EpochNode> queue_;
    std::vector<EpochNode> retired_;
    std::uint32_t current_epoch_{0};
    std::uint32_t cursor_index_{0};
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_EPOCH_QUEUE_HPP
