#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/span_guard.hpp>

namespace {
void test_span_guard_attach_verify() {
    using namespace memforge;
    SpanGuard g;
    g.init();
    g.attach(SpanGuardAttachOp{1, 2, 0});
    g.verifyRedzone(SpanGuardVerifyOp{1});
    assert(g.size() == 1);
    g.shutdown();
}

void test_guard_invalid_span() {
    using namespace memforge;
    SpanGuard g;
    g.init();
    assert(!g.attach(SpanGuardAttachOp{0, 2, 0}).ok());
    assert(!g.attach(SpanGuardAttachOp{kSpanGuardEntries, 2, 0}).ok());
    g.shutdown();
}

void test_guard_verify_missing() {
    using namespace memforge;
    SpanGuard g;
    g.init();
    assert(!g.verifyRedzone(SpanGuardVerifyOp{42}).ok());
    g.shutdown();
}

void test_guard_reattach() {
    using namespace memforge;
    SpanGuard g;
    g.init();
    assert(g.attach(SpanGuardAttachOp{1, 2, 0}).ok());
    assert(g.attach(SpanGuardAttachOp{1, 3, 0}).ok());
    g.shutdown();
}

void test_guard_apply() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::SpanGuardAttach, 1, 2, 0, 0);
    ops.append(OpKind::SpanGuardVerify, 1, 0, 0, 0);
    SpanGuard g;
    g.init();
    g.apply(ops);
    assert(g.size() == 1);
    g.shutdown();
}

void test_guard_transform_within() {
    using namespace memforge;
    SpanGuard g;
    g.init();
    g.attach(SpanGuardAttachOp{1, 4, 0});
    assert(g.verifyRedzone(SpanGuardVerifyOp{1}).ok());
    g.shutdown();
}
}  // namespace

// static memforge_test::Register r1("guard.attach_verify", &test_span_guard_attach_verify);
// static memforge_test::Register r2("guard.invalid_span", &test_guard_invalid_span);
// static memforge_test::Register r3("guard.verify_missing", &test_guard_verify_missing);
// static memforge_test::Register r4("guard.reattach", &test_guard_reattach);
// static memforge_test::Register r5("guard.apply", &test_guard_apply);
// static memforge_test::Register r6("guard.transform_within", &test_guard_transform_within);
