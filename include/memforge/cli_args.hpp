// MemForge - command-line tool arguments parser.
//
// A small, dependency-free argument parser for the memforge CLI.

#ifndef MEMFORGE_CLI_ARGS_HPP
#define MEMFORGE_CLI_ARGS_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace memforge {

struct CliArgs {
    std::string subcommand;
    std::vector<std::string> positional;
    bool verbose{false};
    bool quiet{false};
    bool json{false};
};

class ArgParser {
public:
    ArgParser() = default;
    ~ArgParser() = default;

    CliArgs parse(int argc, const char** argv);
    void print_usage(const char* program) const;
};

}  // namespace memforge

#endif  // MEMFORGE_CLI_ARGS_HPP
