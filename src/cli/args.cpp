// MemForge - CLI argument parser implementation.

#include <memforge/cli_args.hpp>

#include <cstdio>
#include <cstring>

namespace memforge {

CliArgs ArgParser::parse(int argc, const char** argv) {
    CliArgs args{};
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--verbose") {
            args.verbose = true;
        } else if (a == "--quiet") {
            args.quiet = true;
        } else if (a == "--json") {
            args.json = true;
        } else if (a == "--help" || a == "-h") {
            args.subcommand = "help";
        } else if (a.size() > 0 && a[0] == '-') {
            // Unknown flag; ignore.
        } else if (args.subcommand.empty()) {
            args.subcommand = a;
        } else {
            args.positional.push_back(a);
        }
    }
    return args;
}

void ArgParser::print_usage(const char* program) const {
    std::fprintf(stderr,
                 "Usage: %s [--verbose] [--quiet] [--json] <subcommand> [args...]\n"
                 "Subcommands:\n"
                 "  version                 Print library version.\n"
                 "  stats <ops.bin>         Print pool/arena stats.\n"
                 "  inspect <ops.bin>       Dump handle table state.\n"
                 "  migrate <ops.bin>       Run migration plan from ops.\n"
                 "  checkpoint <i> <o>      Write a checkpoint blob.\n"
                 "  validate <ops.bin>      Validate an op stream.\n"
                 "  metrics <ops.bin>       Print hub metrics.\n",
                 program);
}

}  // namespace memforge
