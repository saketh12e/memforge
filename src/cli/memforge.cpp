// MemForge - command-line tool.
//
// Subcommands:
//   memforge version
//   memforge stats <ops.bin>
//   memforge inspect <ops.bin>
//   memforge checkpoint <ops.bin> <out.ckpt>
//   memforge migrate <ops.bin>
//   memforge validate <ops.bin>
//   memforge metrics <ops.bin>
//
// Flags:
//   --verbose     Print extra diagnostics
//   --quiet       Suppress non-error output
//   --json        Emit JSON instead of key=value lines
//   --help        Print usage

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <memforge/api.hpp>
#include <memforge/cli_args.hpp>
#include <memforge/cli_json.hpp>
#include <memforge/op_validator.hpp>

namespace {

std::vector<std::uint8_t> read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    std::vector<std::uint8_t> data{std::istreambuf_iterator<char>(in),
                                   std::istreambuf_iterator<char>()};
    return data;
}

int cmd_version() {
    std::cout << memforge::version_string() << "\n";
    return 0;
}

int cmd_stats(const std::string& path, const memforge::CliArgs& args) {
    auto bytes = read_file(path);
    if (bytes.empty()) {
        std::cerr << "empty input: " << path << "\n";
        return 2;
    }
    memforge::OpStream ops;
    ops.decode(memforge::ConstByteSpan{bytes.data(), bytes.size()});
    memforge::ResourceHub hub;
    hub.init();
    hub.applyOps(ops);
    if (args.json) {
        memforge::JsonWriter jw(stdout);
        jw.begin_object();
        jw.field("ops", static_cast<std::uint64_t>(ops.size()));
        jw.field("arena_blocks", static_cast<std::uint64_t>(hub.arena().blockCount()));
        jw.field("pool_slabs", static_cast<std::uint64_t>(hub.pool().totalSlabs()));
        jw.field("handles", static_cast<std::uint64_t>(hub.handles().size()));
        jw.field("buckets", static_cast<std::uint64_t>(hub.bucketCount()));
        jw.field("guard_entries", static_cast<std::uint64_t>(hub.guards().size()));
        jw.field("batches_pending", static_cast<std::uint64_t>(hub.batches().pending()));
        jw.end_object();
        std::cout << "\n";
    } else {
        std::cout << "ops=" << ops.size()
                  << " arena_blocks=" << hub.arena().blockCount()
                  << " pool_slabs=" << hub.pool().totalSlabs()
                  << " handles=" << hub.handles().size()
                  << " buckets=" << hub.bucketCount()
                  << " guard_entries=" << hub.guards().size()
                  << " batches_pending=" << hub.batches().pending()
                  << "\n";
    }
    hub.destroy();
    return 0;
}

int cmd_inspect(const std::string& path, const memforge::CliArgs& args) {
    auto bytes = read_file(path);
    if (bytes.empty()) {
        std::cerr << "empty input: " << path << "\n";
        return 2;
    }
    memforge::OpStream ops;
    ops.decode(memforge::ConstByteSpan{bytes.data(), bytes.size()});
    memforge::ResourceHub hub;
    hub.init();
    hub.applyOps(ops);
    if (args.json) {
        memforge::JsonWriter jw(stdout);
        jw.begin_object();
        jw.field("handles", static_cast<std::uint64_t>(hub.handles().size()));
        jw.field("pending_defer", static_cast<std::uint64_t>(hub.deferred().size()));
        jw.field("batches", static_cast<std::uint64_t>(hub.batches().pending()));
        jw.field("plan", static_cast<std::uint64_t>(hub.migrator().planSize()));
        jw.field("scope_count", static_cast<std::uint64_t>(hub.scope().scopeCount()));
        jw.field("epoch", static_cast<std::uint64_t>(hub.epoch().epoch()));
        jw.end_object();
        std::cout << "\n";
    } else {
        std::cout << "handles=" << hub.handles().size()
                  << " pending_defer=" << hub.deferred().size()
                  << " batches=" << hub.batches().pending()
                  << " plan=" << hub.migrator().planSize()
                  << " scope_count=" << hub.scope().scopeCount()
                  << " epoch=" << hub.epoch().epoch()
                  << "\n";
        for (std::size_t i = 0; i < hub.handles().size(); ++i) {
            const auto& e = hub.handles().entry(i);
            std::cout << "  handle[" << i << "] slot=" << e.slot_id
                      << " gen=" << e.generation << "\n";
        }
    }
    hub.destroy();
    return 0;
}

int cmd_checkpoint(const std::string& in_path, const std::string& out_path) {
    auto bytes = read_file(in_path);
    if (bytes.empty()) {
        std::cerr << "empty input: " << in_path << "\n";
        return 2;
    }
    memforge::OpStream ops;
    ops.decode(memforge::ConstByteSpan{bytes.data(), bytes.size()});
    memforge::ResourceHub hub;
    hub.init();
    hub.applyOps(ops);
    auto buf = hub.writer().buffer();
    std::ofstream out(out_path, std::ios::binary);
    if (!out) {
        std::cerr << "cannot open output: " << out_path << "\n";
        return 3;
    }
    out.write(reinterpret_cast<const char*>(buf.data()),
              static_cast<std::streamsize>(buf.size()));
    hub.destroy();
    return 0;
}

int cmd_migrate(const std::string& path) {
    auto bytes = read_file(path);
    if (bytes.empty()) {
        std::cerr << "empty input: " << path << "\n";
        return 2;
    }
    memforge::OpStream ops;
    ops.decode(memforge::ConstByteSpan{bytes.data(), bytes.size()});
    memforge::ResourceHub hub;
    hub.init();
    hub.applyOps(ops);
    std::cout << "plan=" << hub.migrator().planSize() << "\n";
    hub.destroy();
    return 0;
}

int cmd_validate(const std::string& path) {
    auto bytes = read_file(path);
    if (bytes.empty()) {
        std::cerr << "empty input: " << path << "\n";
        return 2;
    }
    memforge::OpStream ops;
    ops.decode(memforge::ConstByteSpan{bytes.data(), bytes.size()});
    memforge::OpStreamValidator v;
    auto report = v.validate(ops);
    std::cout << "total=" << report.total
              << " valid=" << report.valid
              << " invalid=" << report.invalid;
    if (report.invalid > 0) {
        std::cout << " first_error=" << report.first_error.message()
                  << " index=" << report.first_error_index;
    }
    std::cout << "\n";
    return report.invalid == 0 ? 0 : 4;
}

int cmd_metrics(const std::string& path) {
    auto bytes = read_file(path);
    if (bytes.empty()) {
        std::cerr << "empty input: " << path << "\n";
        return 2;
    }
    memforge::OpStream ops;
    ops.decode(memforge::ConstByteSpan{bytes.data(), bytes.size()});
    memforge::ResourceHub hub;
    hub.init();
    hub.applyOps(ops);
    auto m = hub.collectMetrics();
    std::cout << "arena.blocks=" << m.arena.blocks
              << " arena.bytes=" << m.arena.total_bytes
              << " arena.fragmented=" << m.arena.fragmented_bytes
              << " pool.classes=" << m.pool.classes
              << " pool.slabs=" << m.pool.slabs
              << " pool.free_slots=" << m.pool.free_slots
              << " pool.total_slots=" << m.pool.total_slots
              << " pool.fragmentation=" << m.pool.fragmentation_ratio
              << " handles.live=" << m.handles.live
              << " epoch.current=" << m.epoch.current
              << " pins.entries=" << m.pins.entries
              << " checkpoint.bytes=" << m.checkpoint.bytes
              << " buckets=" << m.buckets
              << "\n";
    hub.destroy();
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    memforge::ArgParser parser;
    memforge::CliArgs args = parser.parse(argc, const_cast<const char**>(argv));
    if (args.subcommand.empty() || args.subcommand == "help") {
        parser.print_usage(argv[0]);
        return args.subcommand.empty() ? 1 : 0;
    }
    if (args.subcommand == "version") {
        return cmd_version();
    }
    if (args.positional.empty()) {
        parser.print_usage(argv[0]);
        return 1;
    }
    const std::string& in_path = args.positional[0];
    if (args.subcommand == "stats") {
        return cmd_stats(in_path, args);
    }
    if (args.subcommand == "inspect") {
        return cmd_inspect(in_path, args);
    }
    if (args.subcommand == "migrate") {
        return cmd_migrate(in_path);
    }
    if (args.subcommand == "validate") {
        return cmd_validate(in_path);
    }
    if (args.subcommand == "metrics") {
        return cmd_metrics(in_path);
    }
    if (args.subcommand == "checkpoint") {
        if (args.positional.size() < 2) {
            parser.print_usage(argv[0]);
            return 1;
        }
        return cmd_checkpoint(args.positional[0], args.positional[1]);
    }
    parser.print_usage(argv[0]);
    return 1;
}
