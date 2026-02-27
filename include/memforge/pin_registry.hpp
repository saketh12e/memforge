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

class PinRegistry {
public:
    PinRegistry();
    ~PinRegistry();

    PinRegistry(const PinRegistry&) = delete;
    PinRegistry& operator=(const PinRegistry&) = delete;

    Status init();
    void shutdown() noexcept;

    Status pin(const PinOp& op);
    Status releaseAll();

    std::size_t size() const noexcept { return entries_.size(); }

    static std::uint32_t hashPayloadId(std::uint32_t payload_id) noexcept;
    static std::uint32_t aliasLimit() noexcept;

    Status apply(const OpStream& ops);

private:
    std::vector<PinEntry> entries_;
    bool initialized_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_PIN_REGISTRY_HPP
