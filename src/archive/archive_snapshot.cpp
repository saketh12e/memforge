// MemForge - archive snapshot export workflow.

#include <memforge/archive_session.hpp>

#include <cstdlib>
#include <cstring>

namespace memforge {

Status ArchiveSession::exportSnapshot() {
    const ArchiveCopyFrame* frame = findCopyFrame(ArchiveFrameKind::Snapshot);
    if (frame == nullptr || frame->planned_bytes == 0 || frame->data == nullptr) {
        return Ok();
    }
    if (frame->planned_bytes > scratch_capacity_) {
        clearScratch();
        scratch_ = static_cast<std::uint8_t*>(std::malloc(frame->planned_bytes));
        if (scratch_ == nullptr) {
            return Status{ErrorCode::Exhausted};
        }
        scratch_capacity_ = frame->planned_bytes;
    }
    std::memcpy(scratch_, frame->data, frame->planned_bytes);
    return Ok();
}

}  // namespace memforge
