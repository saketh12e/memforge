// MemForge - unified test runner.
//
// Individual test files register their assertions via static initializers.
// The runner executes them in deterministic order and reports pass/fail.

#include <cstdio>
#include <cstdlib>
#include <vector>

#include <memforge/api.hpp>

namespace memforge_test {

struct Test {
    const char* name;
    void (*fn)();
};

inline std::vector<Test>& registry() {
    static std::vector<Test> tests;
    return tests;
}

struct Register {
    Register(const char* name, void (*fn)()) {
        registry().push_back(Test{name, fn});
    }
};

inline int run_all() {
    int failures = 0;
    int total = 0;
    for (const auto& test : registry()) {
        ++total;
        std::printf("[ RUN      ] %s\n", test.name);
        std::fflush(stdout);
        try {
            test.fn();
            std::printf("[       OK ] %s\n", test.name);
            std::fflush(stdout);
        } catch (...) {
            std::printf("[  FAILED  ] %s\n", test.name);
            std::fflush(stdout);
            ++failures;
        }
    }
    std::printf("\n%d/%d tests passed\n", total - failures, total);
    std::fflush(stdout);
    return failures == 0 ? 0 : 1;
}

}  // namespace memforge_test

int main() {
    return memforge_test::run_all();
}
