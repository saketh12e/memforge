// MemForge - non-owning span over a contiguous byte range.
//
// Spans are passed by value; they never free their data. They are the
// foundation of every read/write path that crosses the public boundary.

#ifndef MEMFORGE_SPAN_HPP
#define MEMFORGE_SPAN_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace memforge {

template <typename T>
class Span {
public:
    constexpr Span() noexcept = default;
    constexpr Span(T* data, std::size_t size) noexcept
        : data_(data), size_(size) {}

    template <std::size_t N>
    constexpr Span(T (&array)[N]) noexcept : Span(array, N) {}

    constexpr T* data() const noexcept { return data_; }
    constexpr std::size_t size() const noexcept { return size_; }
    constexpr bool empty() const noexcept { return size_ == 0; }

    constexpr T& operator[](std::size_t index) const noexcept { return data_[index]; }
    constexpr T* begin() const noexcept { return data_; }
    constexpr T* end() const noexcept { return data_ + size_; }

private:
    T* data_{nullptr};
    std::size_t size_{0};
};

using ByteSpan = Span<std::uint8_t>;
using ConstByteSpan = Span<const std::uint8_t>;

}  // namespace memforge

#endif  // MEMFORGE_SPAN_HPP
