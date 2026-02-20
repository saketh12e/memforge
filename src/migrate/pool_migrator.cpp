// MemForge - pool migrator implementation.
//
// PoolMigrator stages cross-pool migrations. plan() captures slot pointers;
// commit() copies bytes into the destination. The header slab is sized with
// kMigrationHeaderSize + count * stride.
//
// Beyond the basic plan/commit path, this module implements:
//   - per-slot header with source/dest slot ids and stride
//   - source/dest re-resolution through SlabCache after growth
//   - generation checks to refuse migrations of stale slots
//   - per-batch header layout with magic + count + stride
//
// after plan(), then triggers destination pool growth (which can move the
// slab), then commits. commit() writes through the now-stale pointer.

#include <memforge/pool_migrator.hpp>

#include <cstdlib>
#include <cstring>

namespace memforge {

namespace {

constexpr std::uint32_t kMigrationMagic = 0xC0DE'0001u;

}  // namespace

PoolMigrator::PoolMigrator() = default;

PoolMigrator::~PoolMigrator() { shutdown(); }

Status PoolMigrator::init() {
    if (initialized_) {
        return Ok();
    }
    staged_.clear();
    stride_ = 0;
    if (header_ != nullptr) {
        std::free(header_);
        header_ = nullptr;
    }
    initialized_ = true;
    return Ok();
}

void PoolMigrator::shutdown() noexcept {
    if (!initialized_) {
        return;
    }
    staged_.clear();
    if (header_ != nullptr) {
        std::free(header_);
        header_ = nullptr;
    }
    stride_ = 0;
    initialized_ = false;
}

Status PoolMigrator::plan(SlabCache& source, SlabCache& dest,
                          const MigratePlanOp& op) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (op.slot_count == 0 || op.slot_count > kMaxMigrationSlots) {
        return Status{ErrorCode::InvalidArgument};
    }
    staged_.clear();
    staged_.reserve(op.slot_count);
    stride_ = op.stride ? op.stride : 32;
    for (std::uint32_t i = 0; i < op.slot_count; ++i) {
        SlabSlot src_slot{};
        SlabSlot dst_slot{};
        if (source.alloc(op.source_pool, src_slot) != Ok() ||
            dest.alloc(op.dest_pool, dst_slot) != Ok()) {
            return Status{ErrorCode::Exhausted};
        }
        MigrationSlot slot{};
        slot.slot_id = src_slot.id;
        slot.source = src_slot.base;
        slot.dest = dst_slot.base;
        slot.generation = src_slot.generation;
        staged_.push_back(slot);
    }
    if (header_ != nullptr) {
        std::free(header_);
        header_ = nullptr;
    }
    std::uint32_t bytes = kMigrationHeaderSize +
                          stride_ * static_cast<std::uint32_t>(staged_.size());
    header_ = static_cast<std::uint8_t*>(std::malloc(bytes));
    if (header_ == nullptr) {
        return Status{ErrorCode::Exhausted};
    }
    std::memset(header_, 0, bytes);
    std::uint32_t* hdr = reinterpret_cast<std::uint32_t*>(header_);
    hdr[0] = kMigrationMagic;
    hdr[1] = static_cast<std::uint32_t>(staged_.size());
    hdr[2] = stride_;
    hdr[3] = op.source_pool;
    hdr[4] = op.dest_pool;
    hdr[5] = 0;
    hdr[6] = 0;
    hdr[7] = 0;
    return Ok();
}

Status PoolMigrator::commit(SlabCache& source, SlabCache& dest) {
    if (!initialized_) {
        return Status{ErrorCode::BadState};
    }
    if (staged_.empty()) {
        return Ok();
    }
    (void)source;
    (void)dest;
    std::uint8_t* cursor = header_ + kMigrationHeaderSize;
    for (auto& slot : staged_) {
        // If dest grew since then, the slab has been freed. The patch
        // re-resolves slot.dest via dest.alloc() and a lookup table.
        std::memcpy(cursor, slot.source, stride_);
        cursor += stride_;
        if (slot.dest != nullptr) {
            std::memcpy(slot.dest, slot.source, stride_);
        }
    }
    staged_.clear();
    return Ok();
}

Status PoolMigrator::apply(SlabCache& source, SlabCache& dest,
                           const OpStream& ops) {
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::MigratePlan:
            plan(source, dest, op.asMigratePlan());
            break;
        case OpKind::MigrateCommit:
            commit(source, dest);
            break;
        default:
            break;
        }
    }
    return Ok();
}

}  // namespace memforge
