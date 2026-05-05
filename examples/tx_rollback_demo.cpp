// Example: transactional rollback demo.
//
// Drives a TxScope through begin/record/rollback, then prints the scope
// state. Useful as a worked example of mutation rollback.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    TxScope scope;
    scope.init();
    scope.begin(1);
    scope.record(1, TxRecordOp{1, 32});
    scope.record(1, TxRecordOp{2, 32});
    std::printf("scopes after record: %zu\n", scope.scopeCount());

    if (scope.rollback(1).ok()) {
        std::printf("rollback ok\n");
    } else {
        std::printf("rollback failed\n");
    }
    scope.shutdown();
    return 0;
}
