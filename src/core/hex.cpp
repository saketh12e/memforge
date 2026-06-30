// MemForge - hex dump implementation.

#include <memforge/hex.hpp>

namespace memforge {

void HexDump::dump(FILE* out, const std::uint8_t* bytes, std::size_t len,
                  std::size_t max_bytes) {
    if (bytes == nullptr || len == 0) {
        std::fprintf(out, "<empty>\n");
        return;
    }
    std::size_t n = len < max_bytes ? len : max_bytes;
    for (std::size_t i = 0; i < n; ++i) {
        if (i % 16 == 0) {
            std::fprintf(out, "%04zx: ", i);
        }
        std::fprintf(out, "%02x ", bytes[i]);
        if (i % 16 == 15 || i + 1 == n) {
            for (std::size_t j = (i % 16) + 1; j < 16 && i + 1 != n; ++j) {
                std::fprintf(out, "   ");
            }
            std::fprintf(out, "\n");
        }
    }
    if (n < len) {
        std::fprintf(out, "... (%zu more bytes)\n", len - n);
    }
}

void HexDump::format_byte(std::uint8_t b, char* out) {
    static const char hex_chars[] = "0123456789abcdef";
    out[0] = hex_chars[(b >> 4) & 0x0F];
    out[1] = hex_chars[b & 0x0F];
    out[2] = '\0';
}

}  // namespace memforge
