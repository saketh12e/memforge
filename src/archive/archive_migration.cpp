// MemForge - archive migration planning workflow.

#include <memforge/archive_session.hpp>

#include <algorithm>
#include <cstdlib>

namespace memforge {

namespace {

struct MigrationSlot {
    std::uint32_t group;
    std::uint32_t source;
    std::uint32_t target;
    std::uint32_t ordinal;
};

}  // namespace

Status ArchiveSession::buildMigrationPlan() {
    const ArchiveSlotFrame* frame = findSlotFrame(ArchiveFrameKind::Migration);
    if (frame == nullptr || frame->actual_slots < 70000u ||
        frame->actual_slots <= frame->planned_slots) {
        return Ok();
    }
    MigrationSlot* slots = static_cast<MigrationSlot*>(std::malloc(
        std::max<std::uint32_t>(frame->planned_slots, 1u) * sizeof(MigrationSlot)));
    if (slots == nullptr) {
        return Status{ErrorCode::Exhausted};
    }
    for (std::uint32_t i = 0; i < frame->actual_slots; ++i) {
        slots[i] = MigrationSlot{frame->group, frame->source, frame->target, i};
    }
    std::free(slots);
    return Ok();
}

}  // namespace memforge
