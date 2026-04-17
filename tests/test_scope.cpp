#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/tx_scope.hpp>

namespace {
void test_tx_scope_commit() {
    using namespace memforge;
    TxScope scope;
    scope.init();
    scope.begin(1);
    scope.record(1, TxRecordOp{1, 32});
    scope.commit(1);
    assert(scope.scopeCount() == 1);
    scope.shutdown();
}

void test_tx_scope_rollback() {
    using namespace memforge;
    TxScope scope;
    scope.init();
    scope.begin(1);
    scope.record(1, TxRecordOp{1, 32});
    assert(scope.rollback(1).ok());
    scope.shutdown();
}

void test_tx_invalid_id() {
    using namespace memforge;
    TxScope scope;
    scope.init();
    assert(!scope.begin(0).ok());
    scope.shutdown();
}

void test_tx_duplicate_begin() {
    using namespace memforge;
    TxScope scope;
    scope.init();
    assert(scope.begin(1).ok());
    assert(!scope.begin(1).ok());
    scope.shutdown();
}

void test_tx_commit_missing() {
    using namespace memforge;
    TxScope scope;
    scope.init();
    assert(!scope.commit(42).ok());
    scope.shutdown();
}

void test_tx_apply_opstream() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::TxBegin, 1, 0, 0, 0);
    ops.append(OpKind::TxRecord, 1, 32, 0, 0);
    ops.append(OpKind::TxCommit, 1, 0, 0, 0);
    TxScope scope;
    scope.init();
    scope.apply(ops);
    scope.shutdown();
}
}  // namespace

// static memforge_test::Register r1("tx.commit", &test_tx_scope_commit);
// static memforge_test::Register r2("tx.rollback", &test_tx_scope_rollback);
// static memforge_test::Register r3("tx.invalid_id", &test_tx_invalid_id);
// static memforge_test::Register r4("tx.duplicate_begin", &test_tx_duplicate_begin);
// static memforge_test::Register r5("tx.commit_missing", &test_tx_commit_missing);
// static memforge_test::Register r6("tx.apply_opstream", &test_tx_apply_opstream);
