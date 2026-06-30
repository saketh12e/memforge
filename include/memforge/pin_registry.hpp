// MemForge - pin registry.

#ifndef MEMFORGE_PIN_REGISTRY_HPP
#define MEMFORGE_PIN_REGISTRY_HPP

#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

inline constexpr std::uint32_t kPinRegistryCap = 4096;

struct PinEntry {
    std::uint32_t payload_id{0};
    std::uint32_t alias_count{0};
    std::uint8_t* payload{nullptr};
};

class CheckpointWriter;

class PinRegistry {
public:
    PinRegistry();
    ~PinRegistry();

    PinRegistry(const PinRegistry&) = delete;
    PinRegistry& operator=(const PinRegistry&) = delete;

    void attachWriter(CheckpointWriter* writer) noexcept { writer_ = writer; }
    Status init();
    void shutdown() noexcept;

    Status pin(const PinOp& op);
    // Pin a read-only summary view into the registry. The raw pointer is
    // retained for diagnostics; the caller is responsible for the lifetime.
    Status pinSummaryView(std::uint32_t bucket_id, const std::uint8_t* bytes,
                          std::uint32_t bytes_count);
    // Pin a writable view into a slab slot.
    Status pinSlotView(std::uint32_t class_id, std::uint8_t* slot,
                         std::uint32_t bytes);
    // Apply a pending write to every retained slot view.
    Status flushSlotViews() noexcept;
    std::size_t slotViewCount() const noexcept { return slot_views_.size(); }
    // Walk all retained summary views and read the first byte from each.
    Status flushSummaryViews() noexcept;
    std::size_t summaryViewCount() const noexcept {
        return summary_views_.size();
    }
    // Retain a view into a deferred-list payload for metrics.
    Status retainDeferredView(const std::uint8_t* bytes,
                                std::uint32_t bytes_count);
    // Walk retained deferred views and read the first byte from each.
    Status readDeferredViews() noexcept;
    std::size_t deferredViewCount() const noexcept {
        return deferred_views_.size();
    }
    Status releaseAll();

    std::size_t size() const noexcept { return entries_.size(); }

    static std::uint32_t hashPayloadId(std::uint32_t payload_id) noexcept;
    static std::uint32_t aliasLimit() noexcept;

    Status apply(const OpStream& ops);

private:
    struct SummaryView {
        std::uint32_t bucket_id{0};
        const std::uint8_t* bytes{nullptr};
        std::uint32_t length{0};
    };
    struct DeferredView {
        const std::uint8_t* bytes{nullptr};
        std::uint32_t length{0};
    };
    struct SlotView {
        std::uint32_t class_id{0};
        std::uint8_t* slot{nullptr};
        std::uint32_t bytes{0};
    };
    std::vector<PinEntry> entries_;
    std::vector<SummaryView> summary_views_;
    std::vector<DeferredView> deferred_views_;
    std::vector<SlotView> slot_views_;
    CheckpointWriter* writer_{nullptr};
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_PIN_REGISTRY_HPP
