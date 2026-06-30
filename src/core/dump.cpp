// MemForge - text dump implementation.

#include <memforge/dump.hpp>
#include <memforge/op_kinds.hpp>

namespace memforge {

void TextDumper::dump_metrics(const HubMetrics& m) {
    std::fprintf(out_, "metrics:\n");
    std::fprintf(out_, "  arena.blocks          = %u\n", m.arena.blocks);
    std::fprintf(out_, "  arena.total_bytes     = %u\n", m.arena.total_bytes);
    std::fprintf(out_, "  arena.fragmented_bytes= %u\n", m.arena.fragmented_bytes);
    std::fprintf(out_, "  pool.classes          = %u\n", m.pool.classes);
    std::fprintf(out_, "  pool.slabs            = %u\n", m.pool.slabs);
    std::fprintf(out_, "  pool.free_slots       = %u\n", m.pool.free_slots);
    std::fprintf(out_, "  pool.total_slots      = %u\n", m.pool.total_slots);
    std::fprintf(out_, "  pool.fragmentation    = %g\n", m.pool.fragmentation_ratio);
    std::fprintf(out_, "  handles.live          = %u\n", m.handles.live);
    std::fprintf(out_, "  handles.obsolete         = %u\n", m.handles.obsolete);
    std::fprintf(out_, "  epoch.current         = %u\n", m.epoch.current);
    std::fprintf(out_, "  epoch.pending         = %u\n", m.epoch.pending);
    std::fprintf(out_, "  epoch.retired         = %u\n", m.epoch.retired);
    std::fprintf(out_, "  pins.entries          = %u\n", m.pins.entries);
    std::fprintf(out_, "  pins.aliases          = %u\n", m.pins.aliases);
    std::fprintf(out_, "  checkpoint.bytes      = %u\n", m.checkpoint.bytes);
    std::fprintf(out_, "  checkpoint.sections   = %u\n", m.checkpoint.sections);
    std::fprintf(out_, "  buckets               = %u\n", m.buckets);
}

void TextDumper::dump_op(const Op& op, std::size_t index) {
    std::fprintf(out_, "[%04zu] %-32s a=%u b=%u c=%u d=%u\n",
                 index, op_kind_name(op.kind),
                 op.a, op.b, op.c, op.d);
}

}  // namespace memforge
