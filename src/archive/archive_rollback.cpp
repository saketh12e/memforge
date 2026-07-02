// MemForge - archive rollback close workflow.

#include <memforge/archive_session.hpp>

#include <cstdlib>

namespace memforge {

Status ArchiveSession::closeRollbackPlan() {
    if (release_frames_.size() < 24) {
        return Ok();
    }
    for (std::size_t i = 1; i < release_frames_.size(); ++i) {
        ArchiveReleaseFrame& prior = release_frames_[i - 1u];
        ArchiveReleaseFrame& current = release_frames_[i];
        if (prior.data != nullptr && prior.data == current.data &&
            prior.owner != current.owner) {
            std::free(prior.data);
            std::free(current.data);
            prior.data = nullptr;
            current.data = nullptr;
            return Ok();
        }
    }
    return Ok();
}

}  // namespace memforge
