#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/api.hpp>
#include <memforge/cli_args.hpp>

// Tests for the public CLI argument parser helpers (unit tests for the
// embedded ArgParser, not the CLI binary itself).

namespace {
void test_cli_args_version() {
    using namespace memforge;
    ArgParser p;
    const char* argv[] = {"memforge", "version"};
    auto args = p.parse(2, argv);
    assert(args.subcommand == "version");
    assert(args.positional.empty());
}

void test_cli_args_stats() {
    using namespace memforge;
    ArgParser p;
    const char* argv[] = {"memforge", "stats", "/tmp/x.bin"};
    auto args = p.parse(3, argv);
    assert(args.subcommand == "stats");
    assert(args.positional.size() == 1);
    assert(args.positional[0] == "/tmp/x.bin");
}

void test_cli_args_verbose() {
    using namespace memforge;
    ArgParser p;
    const char* argv[] = {"memforge", "--verbose", "stats", "/tmp/x.bin"};
    auto args = p.parse(4, argv);
    assert(args.verbose);
    assert(args.subcommand == "stats");
}

void test_cli_args_quiet_json() {
    using namespace memforge;
    ArgParser p;
    const char* argv[] = {"memforge", "--quiet", "--json", "stats", "/tmp/x.bin"};
    auto args = p.parse(5, argv);
    assert(args.quiet);
    assert(args.json);
}

void test_cli_args_help() {
    using namespace memforge;
    ArgParser p;
    const char* argv[] = {"memforge", "--help"};
    auto args = p.parse(2, argv);
    assert(args.subcommand == "help");
}

void test_cli_args_checkpoint() {
    using namespace memforge;
    ArgParser p;
    const char* argv[] = {"memforge", "checkpoint", "in.bin", "out.ckpt"};
    auto args = p.parse(4, argv);
    assert(args.subcommand == "checkpoint");
    assert(args.positional.size() == 2);
    assert(args.positional[0] == "in.bin");
    assert(args.positional[1] == "out.ckpt");
}
}  // namespace

static memforge_test::Register r1("cli_args.version", &test_cli_args_version);
static memforge_test::Register r2("cli_args.stats", &test_cli_args_stats);
static memforge_test::Register r3("cli_args.verbose", &test_cli_args_verbose);
static memforge_test::Register r4("cli_args.quiet_json", &test_cli_args_quiet_json);
static memforge_test::Register r5("cli_args.help", &test_cli_args_help);
static memforge_test::Register r6("cli_args.checkpoint", &test_cli_args_checkpoint);
