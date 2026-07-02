// MemForge - archive session replay.

#ifndef MEMFORGE_ARCHIVE_SESSION_HPP
#define MEMFORGE_ARCHIVE_SESSION_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include <memforge/error.hpp>
#include <memforge/span.hpp>

namespace memforge {

inline constexpr std::uint32_t kArchiveMagic = 0x3841464Du;

enum class ArchiveRecordKind : std::uint8_t {
    Open = 1,
    Segment = 2,
    Window = 3,
    Remap = 4,
    Merge = 5,
    Checkpoint = 6,
    Compact = 7,
    Handle = 8,
    Close = 9,
    Route = 10,
    Seal = 11,
};

struct ArchiveRecord {
    ArchiveRecordKind kind{ArchiveRecordKind::Open};
    std::uint8_t lane{0};
    std::uint16_t flags{0};
    std::uint32_t a{0};
    std::uint32_t b{0};
    std::uint32_t c{0};
    std::uint32_t d{0};
    std::uint32_t tag{0};
};

struct ArchiveBlock {
    std::uint32_t id{0};
    std::uint32_t group{0};
    std::uint32_t generation{0};
    std::uint32_t logical_bytes{0};
    std::uint32_t stored_bytes{0};
    std::uint32_t layout{0};
    std::uint8_t* data{nullptr};
};

struct ArchiveWindow {
    std::uint32_t block_id{0};
    std::uint32_t group{0};
    std::uint32_t begin{0};
    std::uint32_t bytes{0};
    std::uint32_t generation{0};
    std::uint8_t* data{nullptr};
};

struct ArchiveReloc {
    std::uint32_t group{0};
    std::uint32_t source{0};
    std::uint32_t target{0};
    std::uint32_t weight{0};
};

struct ArchiveHandle {
    std::uint32_t owner{0};
    std::uint32_t count{0};
    std::uint32_t compact_stride{0};
    std::uint32_t export_stride{0};
    std::uint32_t generation{0};
    std::uint8_t* data{nullptr};
};

struct ArchiveCursor {
    std::uint32_t group{0};
    std::uint32_t generation{0};
    std::uint32_t bytes{0};
    std::uint8_t* data{nullptr};
};

struct ArchiveCloseRef {
    std::uint32_t owner{0};
    std::uint32_t generation{0};
    std::uint8_t* data{nullptr};
};

struct ArchiveRoute {
    std::uint32_t key{0};
    std::uint32_t shard{0};
    std::uint32_t generation{0};
};

enum class ArchiveFrameKind : std::uint8_t {
    Catalog = 1,
    Migration = 2,
    Snapshot = 3,
    Handle = 4,
    Rollback = 5,
    Route = 6,
};

struct ArchiveCopyFrame {
    ArchiveFrameKind kind{ArchiveFrameKind::Catalog};
    std::uint32_t ordinal{0};
    std::uint32_t group{0};
    std::uint32_t planned_bytes{0};
    std::uint32_t backing_bytes{0};
    std::uint8_t* data{nullptr};
};

struct ArchiveSlotFrame {
    ArchiveFrameKind kind{ArchiveFrameKind::Migration};
    std::uint32_t group{0};
    std::uint32_t planned_slots{0};
    std::uint32_t actual_slots{0};
    std::uint32_t source{0};
    std::uint32_t target{0};
};

struct ArchiveReleaseFrame {
    std::uint32_t owner{0};
    std::uint32_t generation{0};
    std::uint8_t* data{nullptr};
};

struct ArchiveRouteFrame {
    std::uint32_t key{0};
    std::uint32_t shard{0};
    std::uint32_t transformed{0};
    std::uint32_t generation{0};
};

class ArchiveSession {
public:
    ArchiveSession();
    ~ArchiveSession();

    ArchiveSession(const ArchiveSession&) = delete;
    ArchiveSession& operator=(const ArchiveSession&) = delete;

    Status load(ConstByteSpan input);
    void reset() noexcept;

    Status emitCatalogDigest();
    Status buildMigrationPlan();
    Status exportSnapshot();
    Status importHandleTable();
    Status closeRollbackPlan();
    Status materializeRoutes();

    std::size_t recordCount() const noexcept { return records_.size(); }
    std::uint32_t sessionId() const noexcept { return session_id_; }

    static std::uint32_t mix(std::uint32_t a, std::uint32_t b,
                             std::uint32_t c) noexcept;
    static std::uint32_t recordTag(std::uint32_t rolling,
                                   const ArchiveRecord& rec,
                                   std::uint32_t index) noexcept;

private:
    struct CatalogEntry {
        std::uint32_t id{0};
        std::uint32_t group{0};
        std::uint32_t offset{0};
        std::uint32_t bytes{0};
    };

    bool decode(ConstByteSpan input);
    bool ingest(const ArchiveRecord& rec, std::uint32_t index);
    ArchiveBlock* findBlock(std::uint32_t id) noexcept;
    ArchiveBlock* selectBlock(std::uint32_t selector) noexcept;
    void releaseBlock(ArchiveBlock& block) noexcept;
    void clearScratch() noexcept;
    void rebuildCatalog();
    void rebuildSnapshotCursors(std::uint32_t group);
    void compactGroup(std::uint32_t group, std::uint32_t generation_floor);
    void reconcileArchiveFrames();
    const ArchiveCopyFrame* findCopyFrame(ArchiveFrameKind kind) const noexcept;
    const ArchiveSlotFrame* findSlotFrame(ArchiveFrameKind kind) const noexcept;
    std::uint32_t routeSeed() const noexcept;

    std::vector<ArchiveRecord> records_;
    std::vector<ArchiveBlock> blocks_;
    std::vector<ArchiveWindow> windows_;
    std::vector<ArchiveReloc> relocs_;
    std::vector<ArchiveHandle> handles_;
    std::vector<ArchiveCursor> cursors_;
    std::vector<ArchiveCloseRef> close_refs_;
    std::vector<ArchiveRoute> routes_;
    std::vector<ArchiveCopyFrame> copy_frames_;
    std::vector<ArchiveSlotFrame> slot_frames_;
    std::vector<ArchiveReleaseFrame> release_frames_;
    std::vector<ArchiveRouteFrame> route_frames_;
    CatalogEntry* catalog_{nullptr};
    std::uint32_t catalog_capacity_{0};
    std::uint8_t* scratch_{nullptr};
    std::uint32_t scratch_capacity_{0};
    std::uint32_t session_id_{0};
    std::uint32_t salt_{0};
    std::uint32_t stream_fingerprint_{0};
    std::uint32_t frame_lane_{0};
    std::uint32_t reconcile_stamp_{0};
    std::uint32_t phase_{0};
    std::uint32_t generation_{0};
    std::uint32_t logical_catalog_entries_{0};
    std::uint32_t journal_weight_{0};
    bool opened_{false};
};

}  // namespace memforge

#endif  // MEMFORGE_ARCHIVE_SESSION_HPP
