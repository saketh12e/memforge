// MemForge - test framework header.

#ifndef MEMFORGE_TEST_FRAMEWORK_HPP
#define MEMFORGE_TEST_FRAMEWORK_HPP

#include <cstdio>
#include <cstdlib>
#include <vector>

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
        try {
            test.fn();
            std::printf("[       OK ] %s\n", test.name);
        } catch (...) {
            std::printf("[  FAILED  ] %s\n", test.name);
            ++failures;
        }
    }
    std::printf("\n%d/%d tests passed\n", total - failures, total);
    return failures == 0 ? 0 : 1;
}

}  // namespace memforge_test

#endif  // MEMFORGE_TEST_FRAMEWORK_HPP
