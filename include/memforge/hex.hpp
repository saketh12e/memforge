// MemForge - hex dump helper for op stream bytes.

#ifndef MEMFORGE_HEX_HPP
#define MEMFORGE_HEX_HPP

#include <cstdint>
#include <cstdio>
#include <cstddef>

namespace memforge {

class HexDump {
public:
    static void dump(FILE* out, const std::uint8_t* bytes, std::size_t len,
                     std::size_t max_bytes = 256);
    static void format_byte(std::uint8_t b, char* out);
};

}  // namespace memforge

#endif  // MEMFORGE_HEX_HPP
