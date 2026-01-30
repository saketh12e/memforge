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
    std::uint32_t generation_counter_{1};
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_HANDLE_TABLE_HPP
