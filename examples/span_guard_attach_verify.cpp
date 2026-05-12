// Example: span guard attach/verify.
//
// Attaches several spans with different guard classes and verifies each
// redzone.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    SpanGuard guards;
    guards.init();

    for (std::uint32_t i = 1; i <= 4; ++i) {
        guards.attach(SpanGuardAttachOp{i, i, 0});
        Status s = guards.verifyRedzone(SpanGuardVerifyOp{i});
        std::printf("span %u verify: %s\n", i, s.ok() ? "ok" : "fail");
    }
    std::printf("guard entries: %zu\n", guards.size());
    guards.shutdown();
    return 0;
}
