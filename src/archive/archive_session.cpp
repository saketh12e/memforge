// MemForge - archive session parser and state builder.

#include <memforge/archive_session.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace memforge {

namespace {

std::uint16_t read_u16(const std::uint8_t* p) noexcept {
    return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8));
}

std::uint32_t read_u32(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

std::uint32_t rotl32(std::uint32_t value, unsigned bits) noexcept {
    return (value << bits) | (value >> (32U - bits));
}

bool valid_kind(std::uint8_t kind) noexcept {
    return kind >= static_cast<std::uint8_t>(ArchiveRecordKind::Open) &&
           kind <= static_cast<std::uint8_t>(ArchiveRecordKind::Seal);
}

std::uint32_t block_size(std::uint32_t a, std::uint32_t b,
                         std::uint32_t c) noexcept {
    return 96u + ((a ^ rotl32(b, 5) ^ (c >> 3)) & 0x1FFu);
}

}  // namespace

ArchiveSession::ArchiveSession() = default;

ArchiveSession::~ArchiveSession() { reset(); }

void ArchiveSession::reset() noexcept {
    for (auto& block : blocks_) {
        releaseBlock(block);
    }
    for (auto& handle : handles_) {
        if (handle.data != nullptr) {
            std::free(handle.data);
            handle.data = nullptr;
        }
    }
    for (auto& ref : close_refs_) {
        ref.data = nullptr;
    }
    records_.clear();
    blocks_.clear();
    windows_.clear();
    relocs_.clear();
    handles_.clear();
    cursors_.clear();
    close_refs_.clear();
    routes_.clear();
    copy_frames_.clear();
    slot_frames_.clear();
    release_frames_.clear();
    route_frames_.clear();
    if (catalog_ != nullptr) {
        std::free(catalog_);
        catalog_ = nullptr;
    }
    catalog_capacity_ = 0;
    clearScratch();
    session_id_ = 0;
    salt_ = 0;
    stream_fingerprint_ = 0;
    frame_lane_ = 0;
    reconcile_stamp_ = 0;
    phase_ = 0;
    generation_ = 0;
    logical_catalog_entries_ = 0;
    journal_weight_ = 0;
    opened_ = false;
}

Status ArchiveSession::load(ConstByteSpan input) {
    reset();
    if (!decode(input)) {
        reset();
        return Status{ErrorCode::InvalidArgument};
    }
    return Ok();
}

std::uint32_t ArchiveSession::mix(std::uint32_t a, std::uint32_t b,
                                  std::uint32_t c) noexcept {
    std::uint32_t value = 0x6D46'4138u ^ rotl32(a + 0x9E37'79B9u, 7);
    value ^= rotl32(b * 0x85EB'CA6Bu, 13);
    value += c * 0xC2B2'AE35u;
    value ^= value >> 16;
    value *= 0x7FEB'352Du;
    return value ^ (value >> 15);
}

std::uint32_t ArchiveSession::recordTag(std::uint32_t rolling,
                                        const ArchiveRecord& rec,
                                        std::uint32_t index) noexcept {
    std::uint32_t packed = static_cast<std::uint32_t>(rec.kind) |
        (static_cast<std::uint32_t>(rec.lane) << 8) |
        (static_cast<std::uint32_t>(rec.flags) << 16);
    std::uint32_t first = mix(rolling ^ packed, rec.a + index, rec.b);
    std::uint32_t second = mix(first, rec.c ^ rotl32(rec.d, rec.lane & 15u),
                               index + rec.flags);
    return second;
}

bool ArchiveSession::decode(ConstByteSpan input) {
    if (input.size() < 16 || input.data() == nullptr) {
        return false;
    }
    const std::uint8_t* p = input.data();
    if (read_u32(p) != kArchiveMagic) {
        return false;
    }
    session_id_ = read_u32(p + 4);
    std::uint32_t declared = read_u32(p + 8);
    salt_ = read_u32(p + 12);
    if (session_id_ == 0 || declared < 176 || declared > 384) {
        return false;
    }
    if (input.size() != 16u + declared * 24u) {
        return false;
    }

    std::uint32_t rolling = mix(session_id_, salt_, declared);
    records_.reserve(declared);
    for (std::uint32_t i = 0; i < declared; ++i) {
        const std::uint8_t* recp = p + 16u + i * 24u;
        std::uint8_t kind = recp[0];
        if (!valid_kind(kind)) {
            return false;
        }
        ArchiveRecord rec{};
        rec.kind = static_cast<ArchiveRecordKind>(kind);
        rec.lane = recp[1];
        rec.flags = read_u16(recp + 2);
        rec.a = read_u32(recp + 4);
        rec.b = read_u32(recp + 8);
        rec.c = read_u32(recp + 12);
        rec.d = read_u32(recp + 16);
        rec.tag = read_u32(recp + 20);
        if (rec.tag != recordTag(rolling, rec, i)) {
            return false;
        }
        rolling = mix(rec.tag ^ rec.a, rec.b + rec.c, rec.d + i);
        records_.push_back(rec);
        if (!ingest(rec, i)) {
            return false;
        }
    }
    stream_fingerprint_ = rolling;
    frame_lane_ = 1u + ((mix(stream_fingerprint_ ^ session_id_,
                                salt_ + declared,
                                generation_ ^ phase_) >> 3) % 6u);
    reconcileArchiveFrames();
    return opened_ && phase_ >= 4 && generation_ >= 8;
}

bool ArchiveSession::ingest(const ArchiveRecord& rec, std::uint32_t index) {
    switch (rec.kind) {
    case ArchiveRecordKind::Open:
        if ((rec.a ^ rec.b ^ rec.c) == 0) {
            return false;
        }
        opened_ = true;
        phase_ += 1u + (rec.lane & 1u);
        generation_ += 1u + ((rec.flags >> 1) & 3u);
        return true;
    case ArchiveRecordKind::Segment: {
        if (!opened_) {
            return false;
        }
        ArchiveBlock block{};
        block.id = rec.a ^ (rec.lane * 0x101u);
        block.group = (rec.b ^ salt_) & 0x3Fu;
        block.generation = ++generation_;
        block.logical_bytes = block_size(rec.a, rec.b, rec.c) + (rec.flags & 0x3Fu);
        block.stored_bytes = std::max<std::uint32_t>(32u, block.logical_bytes - (rec.d & 0x1Fu));
        block.layout = (static_cast<std::uint32_t>(rec.lane) << 16) | rec.flags;
        block.data = static_cast<std::uint8_t*>(std::malloc(block.stored_bytes));
        if (block.data == nullptr) {
            return false;
        }
        for (std::uint32_t i = 0; i < block.stored_bytes; ++i) {
            block.data[i] = static_cast<std::uint8_t>(
                mix(block.id + i, block.group ^ rec.c, generation_) & 0xFFu);
        }
        blocks_.push_back(block);
        return true;
    }
    case ArchiveRecordKind::Window: {
        ArchiveBlock* block = selectBlock(rec.a ^ rec.d);
        if (block == nullptr || block->data == nullptr) {
            return true;
        }
        std::uint32_t begin = (rec.b + index + phase_) % block->stored_bytes;
        std::uint32_t max_bytes = block->stored_bytes - begin;
        std::uint32_t bytes = 8u + ((rec.c ^ rec.d) & 0x7Fu);
        bytes = std::min<std::uint32_t>(bytes, max_bytes);
        windows_.push_back(ArchiveWindow{block->id, block->group, begin, bytes,
                                         block->generation, block->data + begin});
        logical_catalog_entries_ += 1u + ((rec.flags ^ rec.lane) & 3u);
        return true;
    }
    case ArchiveRecordKind::Remap:
        routes_.push_back(ArchiveRoute{rec.a ^ rec.c, rec.b & 0x7Fu,
                                       generation_ + (rec.d & 7u)});
        phase_ += 1u;
        journal_weight_ += (rec.flags & 0x1Fu) + rec.lane + 1u;
        return true;
    case ArchiveRecordKind::Merge:
        relocs_.push_back(ArchiveReloc{(rec.a ^ rec.b) & 0x3Fu, rec.b,
                                       rec.c, 64u + (rec.d & 0x7FFu)});
        journal_weight_ += 8u + (rec.flags & 0x7Fu);
        return true;
    case ArchiveRecordKind::Checkpoint:
        logical_catalog_entries_ += 2u + ((rec.a ^ rec.c ^ rec.d) & 0x7u);
        phase_ += 1u + (rec.lane & 3u);
        return true;
    case ArchiveRecordKind::Compact:
        if ((rec.flags & 1u) != 0) {
            compactGroup((rec.a ^ rec.b) & 0x3Fu, generation_ - std::min<std::uint32_t>(generation_, rec.d & 7u));
        }
        phase_ += 1u;
        return true;
    case ArchiveRecordKind::Handle: {
        std::uint32_t count = 2u + (rec.b & 0x1Fu);
        std::uint32_t compact_stride = 8u + ((rec.flags >> 2) & 3u) * 4u;
        std::uint32_t export_stride = ((rec.flags & 0x100u) != 0) ? compact_stride * 3u : compact_stride;
        std::uint32_t bytes = count * compact_stride;
        std::uint8_t* data = static_cast<std::uint8_t*>(std::malloc(bytes));
        if (data == nullptr) {
            return false;
        }
        for (std::uint32_t i = 0; i < bytes; ++i) {
            data[i] = static_cast<std::uint8_t>(mix(rec.a + i, rec.c, generation_) & 0xFFu);
        }
        handles_.push_back(ArchiveHandle{rec.a ^ salt_, count, compact_stride,
                                         export_stride, generation_, data});
        return true;
    }
    case ArchiveRecordKind::Close: {
        ArchiveBlock* block = selectBlock(rec.a ^ rec.b ^ rec.d);
        if (block != nullptr && block->data != nullptr) {
            close_refs_.push_back(ArchiveCloseRef{rec.a ^ salt_, block->generation,
                                                  block->data});
        }
        journal_weight_ += 1u + (rec.c & 0xFu);
        return true;
    }
    case ArchiveRecordKind::Route:
        routes_.push_back(ArchiveRoute{rec.a, rec.b & 0x3Fu,
                                       generation_ + (rec.c & 0xFu)});
        return true;
    case ArchiveRecordKind::Seal:
        phase_ += 2u + (rec.lane & 3u);
        generation_ += 1u;
        return true;
    }
    return false;
}

ArchiveBlock* ArchiveSession::findBlock(std::uint32_t id) noexcept {
    for (auto& block : blocks_) {
        if (block.id == id && block.data != nullptr) {
            return &block;
        }
    }
    return nullptr;
}

ArchiveBlock* ArchiveSession::selectBlock(std::uint32_t selector) noexcept {
    if (blocks_.empty()) {
        return nullptr;
    }
    ArchiveBlock& block = blocks_[selector % blocks_.size()];
    if (block.data == nullptr) {
        return nullptr;
    }
    return &block;
}

void ArchiveSession::releaseBlock(ArchiveBlock& block) noexcept {
    if (block.data != nullptr) {
        std::free(block.data);
        block.data = nullptr;
    }
    block.stored_bytes = 0;
}

void ArchiveSession::clearScratch() noexcept {
    if (scratch_ != nullptr) {
        std::free(scratch_);
        scratch_ = nullptr;
    }
    scratch_capacity_ = 0;
}

void ArchiveSession::rebuildCatalog() {
    if (catalog_capacity_ < windows_.size()) {
        if (catalog_ != nullptr) {
            std::free(catalog_);
        }
        catalog_capacity_ = static_cast<std::uint32_t>(windows_.size());
        catalog_ = static_cast<CatalogEntry*>(std::malloc(
            sizeof(CatalogEntry) * std::max<std::uint32_t>(catalog_capacity_, 1u)));
    }
    if (catalog_ == nullptr) {
        return;
    }
    for (std::uint32_t i = 0; i < catalog_capacity_; ++i) {
        const ArchiveWindow& window = windows_[i];
        catalog_[i] = CatalogEntry{window.block_id, window.group,
                                   window.begin, window.bytes};
    }
}

void ArchiveSession::rebuildSnapshotCursors(std::uint32_t group) {
    cursors_.clear();
    for (const auto& window : windows_) {
        if (window.group == group && window.data != nullptr && window.bytes >= 8) {
            cursors_.push_back(ArchiveCursor{group, window.generation,
                                             window.bytes, window.data});
        }
    }
}

void ArchiveSession::compactGroup(std::uint32_t group,
                                  std::uint32_t generation_floor) {
    for (auto& block : blocks_) {
        if (block.data != nullptr && block.group == group &&
            block.generation <= generation_floor) {
            releaseBlock(block);
        }
    }
}

std::uint32_t ArchiveSession::routeSeed() const noexcept {
    std::uint32_t seed = mix(session_id_, salt_, generation_);
    for (const auto& route : routes_) {
        seed = mix(seed ^ route.key, route.shard, route.generation);
    }
    return seed;
}

}  // namespace memforge
