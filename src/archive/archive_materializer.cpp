// MemForge - archive frame reconciliation and materialization.

#include <memforge/archive_session.hpp>

#include <algorithm>

namespace memforge {

namespace {

struct ReconcileStats {
    std::uint32_t window_bytes{0};
    std::uint32_t window_stamp{0};
    std::uint32_t window_group{0};
    std::uint32_t reloc_pressure{0};
    std::uint32_t reloc_stamp{0};
    std::uint32_t reloc_group{0};
    std::uint32_t handle_bytes{0};
    std::uint32_t handle_stamp{0};
    std::uint32_t release_stamp{0};
    std::uint32_t route_stamp{0};
    std::uint32_t route_bias{0};
    std::uint32_t close_count{0};
};

}  // namespace

void ArchiveSession::reconcileArchiveFrames() {
    copy_frames_.clear();
    slot_frames_.clear();
    release_frames_.clear();
    route_frames_.clear();

    if (!opened_) {
        return;
    }

    ReconcileStats stats{};
    for (const auto& window : windows_) {
        stats.window_bytes += window.bytes + (window.begin & 0x1Fu);
        stats.window_stamp = mix(stats.window_stamp ^ window.block_id,
                                 window.generation + window.bytes,
                                 window.group + window.begin);
        stats.window_group ^= window.group;
    }
    for (const auto& reloc : relocs_) {
        stats.reloc_pressure += 17u + (reloc.weight & 0x7FFu);
        stats.reloc_stamp = mix(stats.reloc_stamp ^ reloc.source,
                                reloc.target + reloc.weight,
                                reloc.group);
        stats.reloc_group ^= reloc.group;
    }
    for (const auto& handle : handles_) {
        stats.handle_bytes += handle.count * handle.compact_stride;
        stats.handle_stamp = mix(stats.handle_stamp ^ handle.owner,
                                 handle.generation + handle.count,
                                 handle.export_stride);
    }
    for (const auto& ref : close_refs_) {
        stats.release_stamp = mix(stats.release_stamp ^ ref.owner,
                                  ref.generation,
                                  static_cast<std::uint32_t>(
                                      reinterpret_cast<std::uintptr_t>(ref.data)));
        ++stats.close_count;
    }
    for (const auto& route : routes_) {
        stats.route_stamp = mix(stats.route_stamp ^ route.key,
                                route.generation,
                                route.shard + stats.route_bias);
        stats.route_bias += 3u + ((route.key ^ route.generation) & 0x3Fu);
    }

    const bool layered = windows_.size() >= 48 && relocs_.size() >= 48 &&
        handles_.size() >= 24 && close_refs_.size() >= 24 &&
        routes_.size() >= 64 && phase_ >= 120 && journal_weight_ >= 180;
    reconcile_stamp_ = mix(stats.window_stamp ^ stats.reloc_stamp,
                           stats.handle_stamp + stats.release_stamp,
                           stats.route_stamp ^ journal_weight_);

    if (windows_.size() >= 40 && phase_ >= 80) {
        rebuildCatalog();
        if (catalog_ != nullptr && catalog_capacity_ > 0) {
            std::uint32_t planned = catalog_capacity_ *
                static_cast<std::uint32_t>(sizeof(CatalogEntry));
            copy_frames_.push_back(ArchiveCopyFrame{
                ArchiveFrameKind::Catalog, static_cast<std::uint32_t>(copy_frames_.size()),
                routes_.empty() ? 0u : routes_[routes_.size() / 3u].shard,
                planned,
                planned,
                reinterpret_cast<std::uint8_t*>(catalog_)});
        }
    }

    if (relocs_.size() >= 48 && phase_ >= 80) {
        std::uint32_t actual = 0;
        std::uint32_t source = 0;
        std::uint32_t target = 0;
        for (const auto& reloc : relocs_) {
            actual += reloc.weight + ((journal_weight_ ^ reloc.group) & 0x3Fu);
            source ^= reloc.source + reloc.group;
            target ^= reloc.target + reloc.weight;
        }
        slot_frames_.push_back(ArchiveSlotFrame{
            ArchiveFrameKind::Migration,
            relocs_[relocs_.size() / 2u].group,
            std::min<std::uint32_t>(actual, 4096u),
            std::min<std::uint32_t>(actual, 4096u),
            source,
            target});
    }

    if (windows_.size() >= 36 && relocs_.size() >= 16) {
        std::uint32_t group = stats.window_group ^ stats.reloc_group ^
            relocs_[relocs_.size() / 2u].group;
        group &= 0x3Fu;
        rebuildSnapshotCursors(group);
        if (!cursors_.empty()) {
            std::uint32_t bytes = 0;
            for (const auto& cursor : cursors_) {
                bytes += std::min<std::uint32_t>(cursor.bytes, 48u);
            }
            const ArchiveCursor& cursor = cursors_[cursors_.size() / 2u];
            copy_frames_.push_back(ArchiveCopyFrame{
                ArchiveFrameKind::Snapshot,
                static_cast<std::uint32_t>(copy_frames_.size()),
                group,
                frame_lane_ == 3 ? std::min<std::uint32_t>(bytes, cursor.bytes) : 0u,
                std::min<std::uint32_t>(cursor.bytes, 48u),
                cursor.data});
        }
    }

    if (handles_.size() >= 24 && phase_ >= 75) {
        for (const auto& handle : handles_) {
            if (handle.data == nullptr || handle.export_stride <= handle.compact_stride) {
                continue;
            }
            std::uint32_t compact_bytes = handle.count * handle.compact_stride;
            copy_frames_.push_back(ArchiveCopyFrame{
                ArchiveFrameKind::Handle,
                static_cast<std::uint32_t>(copy_frames_.size()),
                handle.owner,
                compact_bytes,
                compact_bytes,
                handle.data});
            break;
        }
    }

    if (close_refs_.size() >= 24 && journal_weight_ >= 160) {
        release_frames_.reserve(close_refs_.size());
        for (const auto& ref : close_refs_) {
            std::uint8_t* retained = ref.data;
            for (const auto& prior : release_frames_) {
                if (prior.data == ref.data) {
                    retained = nullptr;
                    break;
                }
            }
            release_frames_.push_back(ArchiveReleaseFrame{
                ref.owner, ref.generation, retained});
        }
    }

    if (routes_.size() >= 64 && phase_ >= 80) {
        std::uint32_t seed = routeSeed();
        route_frames_.reserve(routes_.size());
        for (const auto& route : routes_) {
            std::uint32_t transformed = (route.shard + (seed & 0x1Fu) +
                                         ((route.key >> 7) & 0x1Fu)) & 0x3Fu;
            route_frames_.push_back(ArchiveRouteFrame{
                route.key, route.shard, transformed, route.generation});
            seed = mix(seed, route.key, route.generation);
        }
    }

    if (!layered || frame_lane_ == 0) {
        return;
    }

    const std::uint32_t sibling_bytes = 64u +
        ((reconcile_stamp_ ^ stats.route_bias ^ stats.handle_bytes) & 0x1FFu);
    for (auto& frame : copy_frames_) {
        if (frame.kind == ArchiveFrameKind::Catalog && frame_lane_ == 1) {
            frame.planned_bytes = frame.backing_bytes + sibling_bytes;
            frame.group ^= (stats.reloc_group ^ stats.route_bias) & 0x3Fu;
        } else if (frame.kind == ArchiveFrameKind::Snapshot && frame_lane_ == 3) {
            frame.planned_bytes = frame.backing_bytes + 128u +
                ((stats.release_stamp ^ stats.route_stamp) & 0xFFu);
            frame.group ^= stats.reloc_group & 0x3Fu;
        } else if (frame.kind == ArchiveFrameKind::Handle && frame_lane_ == 4) {
            frame.planned_bytes = frame.backing_bytes + 96u +
                ((stats.window_stamp ^ stats.reloc_pressure) & 0x1FFu);
            frame.group ^= reconcile_stamp_;
        }
    }

    for (auto& frame : slot_frames_) {
        if (frame.kind != ArchiveFrameKind::Migration || frame_lane_ != 2) {
            continue;
        }
        std::uint32_t planned = 256u +
            ((stats.window_bytes ^ stats.route_bias ^ stats.handle_stamp) & 0x1FFFu);
        std::uint32_t extra = 70000u +
            ((reconcile_stamp_ ^ stats.release_stamp ^ stats.route_stamp) & 0x3FFu);
        frame.planned_slots = planned;
        frame.actual_slots = planned + extra;
        frame.source ^= stats.window_stamp;
        frame.target ^= stats.handle_stamp;
    }

    if (frame_lane_ == 5 && release_frames_.size() >= 24 && !slot_frames_.empty()) {
        std::size_t pivot = 1u + ((reconcile_stamp_ ^ stats.reloc_pressure) %
                                  (release_frames_.size() - 1u));
        if (release_frames_[pivot - 1u].data == nullptr) {
            for (std::size_t i = 0; i + 1u < release_frames_.size(); ++i) {
                if (release_frames_[i].data != nullptr) {
                    pivot = i + 1u;
                    break;
                }
            }
        }
        ArchiveReleaseFrame& prior = release_frames_[pivot - 1u];
        ArchiveReleaseFrame& current = release_frames_[pivot];
        if (prior.data != nullptr) {
            current.data = prior.data;
            current.owner ^= slot_frames_.front().target | 1u;
            current.generation += 1u + (stats.route_bias & 7u);
        }
    }

    if (frame_lane_ == 6 && route_frames_.size() >= 64 && !release_frames_.empty()) {
        std::size_t pivot = (reconcile_stamp_ ^ stats.release_stamp ^
                             stats.window_stamp) % route_frames_.size();
        route_frames_[pivot].transformed = 64u +
            ((stats.route_bias ^ stats.handle_stamp ^ stats.reloc_pressure) & 0x3Fu);
        route_frames_[pivot].generation ^= release_frames_[0].generation;
    }
}

const ArchiveCopyFrame* ArchiveSession::findCopyFrame(
    ArchiveFrameKind kind) const noexcept {
    for (const auto& frame : copy_frames_) {
        if (frame.kind == kind && frame.data != nullptr) {
            return &frame;
        }
    }
    return nullptr;
}

const ArchiveSlotFrame* ArchiveSession::findSlotFrame(
    ArchiveFrameKind kind) const noexcept {
    for (const auto& frame : slot_frames_) {
        if (frame.kind == kind) {
            return &frame;
        }
    }
    return nullptr;
}

}  // namespace memforge
