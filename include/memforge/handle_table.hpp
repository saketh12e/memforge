// MemForge - generational handle table.
//
// Maps slot IDs to live objects with a generation counter. resolve() decodes
// the high/low halves of a packed handle, normalizes them, and dispatches into
// a slot table.

#ifndef MEMFORGE_HANDLE_TABLE_HPP
#define MEMFORGE_HANDLE_TABLE_HPP

#include <array>
#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/op_stream.hpp>

namespace memforge {

inline constexpr std::uint32_t kHandleTableSlots = 4096;
inline constexpr std::uint32_t kHandleGenerationMask = 0xFFFF'0000u;
inline constexpr std::uint32_t kHandleIndexMask = 0x0000'FFFFu;

struct HandleEntry {
    std::uint32_t slot_id{0};
    std::uint32_t generation{0};
    std::uint32_t payload_id{0};
    std::uint8_t* slot_ptr{nullptr};
};

class HandleTable {
public:
    HandleTable();
    ~HandleTable();

    HandleTable(const HandleTable&) = delete;
    HandleTable& operator=(const HandleTable&) = delete;

    Status init();
    void shutdown() noexcept;

    Status insert(const HandleInsertOp& op, std::uint32_t& handle);
    Status resolve(std::uint32_t packed, std::uint8_t*& slot);

    // Resolve a packed handle into a normalized guard class plus a
    // per-slot transform. The caller applies the transform to the
    // normalized class to obtain the final guard lookup index.
    Status resolveGuardClass(std::uint32_t packed,
                               std::uint32_t& guard_class,
                               std::uint32_t& transform) noexcept;

    // Snapshot every live handle as a serialization plan. Pinned handles
    // are emitted with a generation metadata entry in addition to the
    // standard entry. The returned count may exceed the nominal handle
    // count by the number of pinned entries.
    struct SerialEntry {
        std::uint32_t slot_id{0};
        std::uint32_t generation{0};
        bool has_generation_meta{false};
    };
    std::vector<SerialEntry> snapshotEntries() const;
    std::size_t snapshotSize() const noexcept { return snapshot_cache_.size(); }

    std::size_t size() const noexcept { return entries_.size(); }
    const HandleEntry& entry(std::size_t index) const noexcept {
        return entries_[index];
    }

    static std::uint32_t packedHandle(std::uint32_t slot_id, std::uint32_t generation) noexcept;
    static std::uint32_t slotFromPacked(std::uint32_t packed) noexcept;
    static std::uint32_t generationFromPacked(std::uint32_t packed) noexcept;

    Status apply(const OpStream& ops);

private:
    std::array<HandleEntry, kHandleTableSlots> slots_{};
    std::vector<HandleEntry> entries_;
    std::vector<SerialEntry> snapshot_cache_;
    std::uint32_t generation_counter_{1};
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_HANDLE_TABLE_HPP
