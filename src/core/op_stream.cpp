// MemForge - op stream decoder/encoder.
//
// The wire format is little-endian and packed as a sequence of records:
//   uint16 kind, uint16 flags, uint32 a, uint32 b, uint32 c, uint32 d
//
// Each record is 20 bytes. The decoder truncates trailing partial records
// silently to allow fuzz inputs of any length.

#include <memforge/op_stream.hpp>

#include <cstring>
#include <type_traits>

namespace memforge {

namespace {

constexpr std::size_t kOpRecordSize = 20;

std::uint16_t read_u16(const std::uint8_t* p) noexcept {
    std::uint16_t v = 0;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

std::uint32_t read_u32(const std::uint8_t* p) noexcept {
    std::uint32_t v = 0;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

void write_u16(std::uint8_t* p, std::uint16_t v) noexcept {
    std::memcpy(p, &v, sizeof(v));
}

void write_u32(std::uint8_t* p, std::uint32_t v) noexcept {
    std::memcpy(p, &v, sizeof(v));
}

}  // namespace

Status OpStream::decode(ConstByteSpan bytes) noexcept {
    ops_.clear();
    if (bytes.empty()) {
        return Ok();
    }

    const std::uint8_t* cursor = bytes.data();
    std::size_t remaining = bytes.size();

    while (remaining >= kOpRecordSize) {
        std::uint16_t kind = read_u16(cursor);
        std::uint16_t flags = read_u16(cursor + 2);
        std::uint32_t a = read_u32(cursor + 4);
        std::uint32_t b = read_u32(cursor + 8);
        std::uint32_t c = read_u32(cursor + 12);
        std::uint32_t d = read_u32(cursor + 16);
        (void)flags;

        ops_.push_back(Op{static_cast<OpKind>(kind), a, b, c, d});
        cursor += kOpRecordSize;
        remaining -= kOpRecordSize;
    }

    return Ok();
}

Status OpStream::encode(std::vector<std::uint8_t>& out) const noexcept {
    out.clear();
    out.reserve(ops_.size() * kOpRecordSize);
    for (const Op& op : ops_) {
        std::uint8_t record[kOpRecordSize] = {};
        write_u16(record, static_cast<std::uint16_t>(op.kind));
        write_u16(record + 2, 0);
        write_u32(record + 4, op.a);
        write_u32(record + 8, op.b);
        write_u32(record + 12, op.c);
        write_u32(record + 16, op.d);
        out.insert(out.end(), record, record + kOpRecordSize);
    }
    return Ok();
}

}  // namespace memforge
