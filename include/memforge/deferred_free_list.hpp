// MemForge - deferred free list.
//
// A separate, lighter-weight queue used by the ResourceHub for per-class
// deferred frees. flush() walks the list; compact() drops tombstones.

#ifndef MEMFORGE_DEFERRED_FREE_LIST_HPP
#define MEMFORGE_DEFERRED_FREE_LIST_HPP

#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

inline constexpr std::uint32_t kDeferListCap = 4096;

struct DeferredNode {
    std::uint32_t id{0};
    std::uint32_t size_hint{0};
    std::uint8_t* payload{nullptr};
    bool tombstone{false};
};

class DeferredFreeList {
public:
    DeferredFreeList();
    ~DeferredFreeList();

    DeferredFreeList(const DeferredFreeList&) = delete;
    DeferredFreeList& operator=(const DeferredFreeList&) = delete;

    Status init();
    void shutdown() noexcept;

    Status push(const DeferPushOp& op);
    Status compact();
    Status flush();

    std::size_t size() const noexcept { return nodes_.size(); }

    static std::uint32_t hashNodeId(std::uint32_t id) noexcept;
    static std::uint32_t retentionWindow() noexcept;

    Status apply(const OpStream& ops);

private:
    std::vector<DeferredNode> nodes_;
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_DEFERRED_FREE_LIST_HPP
