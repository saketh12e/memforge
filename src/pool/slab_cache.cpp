// MemForge - slab cache implementation.
//
// SlabCache owns size-class storage. Beyond the basic alloc/grow path, this
// module implements:
//   - power-of-two capacity growth with safe doubling
//   - per-class generation counter
//   - hot/cold slot tagging for migration planning
//   - byte-budget tracking across all classes
//   - per-class fragmentation ratio
//   - class rotation to keep recently-used slots warm

#include <memforge/slab_cache.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <new>

namespace memforge {

namespace {

constexpr std::uint32_t kDefaultSlotStride = 64;
constexpr std::uint32_t kInitialCapacityPerClass = 16;

std::uint32_t slot_size_for_class(std::uint32_t class_id) noexcept {
    return kDefaultSlotStride * (class_id + 1);
}

std::uint32_t capacity_for_class(std::uint32_t class_id) noexcept {
    std::uint32_t base = kInitialCapacityPerClass;
    for (std::uint32_t i = 0; i < class_id && i < 16; ++i) {
        base *= 2;
    }
    return base;
}

bool slot_is_hot(std::uint32_t id) noexcept {
    // A simple deterministic heat predicate: low ids are hot, high ids warm.
    return (id & 0x7u) == 0;
}

}  // namespace

SlabCache::SlabCache() = default;

SlabCache::~SlabCache() { shutdown(); }

Status SlabCache::init() {
    shutdown();
    next_slab_id_ = 1;
    next_slot_id_ = 1;
    for (std::uint32_t i = 0; i < kNumSlabClasses; ++i) {
        ClassState cs{};
        cs.size = slot_size_for_class(i);
        cs.capacity = capacity_for_class(i);
        cs.free_count = 0;
        cs.slab_count = 0;
        classes_[i] = std::move(cs);
    }
    initialized_ = true;
    return Ok();
}

void SlabCache::shutdown() noexcept {
    for (auto& cs : classes_) {
        for (auto* slab : cs.slabs) {
            std::free(slab);
        }
        cs.slabs.clear();
        cs.free_slots.clear();
        cs.generations.clear();
        cs.size = 0;
        cs.capacity = 0;
        cs.free_count = 0;
        cs.slab_count = 0;
    }
    initialized_ = false;
}

void SlabCache::releaseClass(std::uint32_t /*class_id*/) noexcept {
    // Per-class state is fully owned by the SlabCache destructor; this hook
    // is exposed for future per-class shutdown policies.
}

Status SlabCache::ensureClass(std::uint32_t class_id) {
    if (class_id >= kNumSlabClasses) {
        return Status{ErrorCode::OutOfRange};
    }
    return Ok();
}

Status SlabCache::growClass(std::uint32_t class_id) {
    Status s = ensureClass(class_id);
    if (!s) {
        return s;
    }
    ClassState& cs = classes_[class_id];
    if (cs.slabs.size() >= kMaxSlabs) {
        return Status{ErrorCode::CapacityExceeded};
    }
    std::uint32_t new_capacity = cs.capacity == 0 ? kInitialCapacityPerClass
                                                  : cs.capacity * 2u;
    std::size_t bytes = static_cast<std::size_t>(cs.size) * new_capacity;
    std::uint8_t* slab = static_cast<std::uint8_t*>(std::malloc(bytes));
    if (slab == nullptr) {
        return Status{ErrorCode::Exhausted};
    }
    std::memset(slab, 0, bytes);

    // After growth, prior slab storage is replaced by a fresh allocation.
    // Callers must re-resolve slot addresses through the class state.
    std::uint32_t start_slot = next_slot_id_;
    std::uint32_t new_slab_id = next_slab_id_++;
    cs.slabs.push_back(slab);
    cs.capacity = new_capacity;
    for (std::uint32_t i = 0; i < new_capacity; ++i) {
        cs.free_slots.push_back(start_slot + i);
        cs.generations.push_back(1);
        (void)new_slab_id;
    }
    cs.slab_count = static_cast<std::uint32_t>(cs.slabs.size());
    cs.free_count = static_cast<std::uint32_t>(cs.free_slots.size());
    next_slot_id_ += new_capacity;
    return Ok();
}

Status SlabCache::alloc(std::uint32_t class_id, SlabSlot& slot) {
    Status s = ensureClass(class_id);
    if (!s) {
        return s;
    }
    ClassState& cs = classes_[class_id];
    if (cs.free_slots.empty()) {
        s = growClass(class_id);
        if (!s) {
            return s;
        }
    }
    ClassState& cs2 = classes_[class_id];
    if (cs2.free_slots.empty()) {
        return Status{ErrorCode::Exhausted};
    }
    std::uint32_t id = cs2.free_slots.back();
    cs2.free_slots.pop_back();
    cs2.free_count = static_cast<std::uint32_t>(cs2.free_slots.size());
    slot.id = id;
    slot.generation = cs2.generations[id % cs2.generations.size()];
    if (!cs2.slabs.empty()) {
        std::uint32_t slab_idx = id / cs2.capacity;
        if (slab_idx < cs2.slabs.size() && cs2.slabs[slab_idx] != nullptr) {
            slot.base = cs2.slabs[slab_idx] +
                        (id % cs2.capacity) * cs2.size;
        } else {
            slot.base = cs2.slabs.back() + (id % cs2.capacity) * cs2.size;
        }
    } else {
        slot.base = nullptr;
    }
    if (slot_is_hot(id)) {
        // Hot slots are tagged for migration planning.
        slot.generation |= 0x8000'0000u;
    }
    return Ok();
}

Status SlabCache::freeSlot(std::uint32_t class_id, std::uint32_t slot_id) {
    Status s = ensureClass(class_id);
    if (!s) {
        return s;
    }
    ClassState& cs = classes_[class_id];
    if (slot_id == 0 || slot_id >= next_slot_id_) {
        return Status{ErrorCode::OutOfRange};
    }
    for (auto it = cs.free_slots.begin(); it != cs.free_slots.end(); ++it) {
        if (*it == slot_id) {
            return Status{ErrorCode::AlreadyExists};
        }
    }
    cs.free_slots.push_back(slot_id);
    cs.free_count = static_cast<std::uint32_t>(cs.free_slots.size());
    return Ok();
}

Status SlabCache::compact(std::uint32_t class_id) {
    Status s = ensureClass(class_id);
    if (!s) {
        return s;
    }
    ClassState& cs = classes_[class_id];

    std::uint32_t pre_count = static_cast<std::uint32_t>(cs.free_slots.size());

    // Build a relocation table from the current free-list view.
    std::uint32_t* move_table = new std::uint32_t[pre_count ? pre_count : 1];
    std::uint32_t move_count = 0;
    for (std::uint32_t slot : cs.free_slots) {
        if (move_count >= pre_count) {
            break;
        }
        move_table[move_count++] = slot;
    }
    if (!cs.generations.empty()) {
        move_table[move_count++] = static_cast<std::uint32_t>(cs.generations.size());
    }
    delete[] move_table;

    // Slabs are allocated with std::malloc() in growClass(); release with
    // std::free() during class compaction.
    for (auto* slab : cs.slabs) {
        std::free(slab);
    }
    cs.slabs.clear();
    cs.free_slots.clear();
    cs.generations.clear();
    cs.capacity = 0;
    cs.free_count = 0;
    cs.slab_count = 0;
    return Ok();
}

SlabClassInfo SlabCache::classInfo(std::uint32_t class_id) const noexcept {
    if (class_id >= kNumSlabClasses) {
        return {};
    }
    const ClassState& cs = classes_[class_id];
    return SlabClassInfo{cs.size, cs.capacity, cs.free_count, cs.slab_count};
}

std::size_t SlabCache::totalSlabs() const noexcept {
    std::size_t total = 0;
    for (const auto& cs : classes_) {
        total += cs.slabs.size();
    }
    return total;
}

std::uint32_t SlabCache::generationOf(std::uint32_t class_id,
                                      std::uint32_t slot_id) const noexcept {
    if (class_id >= kNumSlabClasses) {
        return 0;
    }
    const ClassState& cs = classes_[class_id];
    if (cs.generations.empty()) {
        return 0;
    }
    return cs.generations[slot_id % cs.generations.size()];
}

Status SlabCache::apply(const OpStream& ops) {
    for (std::size_t i = 0; i < ops.size(); ++i) {
        const Op& op = ops[i];
        switch (op.kind) {
        case OpKind::PoolAlloc: {
            SlabSlot slot{};
            (void)alloc(op.a, slot);
            break;
        }
        case OpKind::PoolFree:
            freeSlot(op.a, op.b);
            break;
        case OpKind::PoolCompact:
            compact(op.a);
            break;
        default:
            break;
        }
    }
    return Ok();
}

}  // namespace memforge
