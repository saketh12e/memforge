#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#ifndef MEMFORGE_CLI_PATH
#define MEMFORGE_CLI_PATH "memforge_cli"
#endif

#include "test_framework.hpp"
#include <memforge/api.hpp>

namespace {
void test_cli_version() {
    const std::string cli = std::string(MEMFORGE_CLI_PATH);
    if (std::system(("which " + cli + " > /dev/null 2>&1").c_str()) != 0) {
        std::printf("[ SKIP     ] cli not on PATH\n");
        return;
    }
    std::string cmd = cli + " version > /dev/null 2>&1";
    assert(std::system(cmd.c_str()) == 0);
}

void test_cli_help() {
    const std::string cli = std::string(MEMFORGE_CLI_PATH);
    if (std::system(("which " + cli + " > /dev/null 2>&1").c_str()) != 0) {
        std::printf("[ SKIP     ] cli not on PATH\n");
        return;
    }
    std::string cmd = cli + " > /dev/null 2>&1";
    int rc = std::system(cmd.c_str());
    assert(rc != 0);
}
}  // namespace

// static memforge_test::Register r1("cli.version", &test_cli_version);
// static memforge_test::Register r2("cli.help", &test_cli_help);
