// MemForge - archive route materialization workflow.

#include <memforge/archive_session.hpp>

#include <cstdlib>

namespace memforge {

namespace {

struct RouteCell {
    std::uint32_t key;
    std::uint32_t shard;
    std::uint32_t generation;
    std::uint32_t checksum;
};

}  // namespace

Status ArchiveSession::materializeRoutes() {
    if (route_frames_.size() < 64) {
        return Ok();
    }
    constexpr std::uint32_t capacity = 64;
    RouteCell* table = static_cast<RouteCell*>(std::malloc(capacity * sizeof(RouteCell)));
    if (table == nullptr) {
        return Status{ErrorCode::Exhausted};
    }
    for (std::uint32_t i = 0; i < capacity; ++i) {
        table[i] = RouteCell{0, i, generation_, mix(session_id_, i, salt_)};
    }
    for (const auto& frame : route_frames_) {
        table[frame.transformed] = RouteCell{
            frame.key, frame.shard, frame.generation,
            mix(session_id_ ^ frame.key, frame.transformed, frame.generation)};
    }
    std::free(table);
    return Ok();
}

}  // namespace memforge
