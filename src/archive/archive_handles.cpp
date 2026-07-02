// MemForge - archive handle import workflow.

#include <memforge/archive_session.hpp>

#include <cstdlib>
#include <cstring>

namespace memforge {

Status ArchiveSession::importHandleTable() {
    const ArchiveCopyFrame* frame = findCopyFrame(ArchiveFrameKind::Handle);
    if (frame == nullptr || frame->planned_bytes <= frame->backing_bytes + 32u) {
        return Ok();
    }
    clearScratch();
    scratch_ = static_cast<std::uint8_t*>(std::malloc(frame->backing_bytes));
    if (scratch_ == nullptr) {
        return Status{ErrorCode::Exhausted};
    }
    scratch_capacity_ = frame->backing_bytes;
    std::memcpy(scratch_, frame->data, frame->planned_bytes);
    return Ok();
}

}  // namespace memforge
