// MemForge - archive catalog export workflow.

#include <memforge/archive_session.hpp>

#include <cstdlib>
#include <cstring>

namespace memforge {

Status ArchiveSession::emitCatalogDigest() {
    const ArchiveCopyFrame* frame = findCopyFrame(ArchiveFrameKind::Catalog);
    if (frame == nullptr || frame->planned_bytes <= frame->backing_bytes + 8u) {
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
