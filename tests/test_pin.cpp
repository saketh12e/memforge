#include <cassert>
#include <cstdio>

#include "test_framework.hpp"
#include <memforge/pin_registry.hpp>

namespace {
void test_pin_release() {
    using namespace memforge;
    PinRegistry pins;
    pins.init();
    pins.pin(PinOp{1, 0});
    pins.releaseAll();
    assert(pins.size() == 0);
    pins.shutdown();
}

void test_pin_capacity() {
    using namespace memforge;
    PinRegistry pins;
    pins.init();
    for (std::uint32_t i = 0; i < kPinRegistryCap; ++i) {
        assert(pins.pin(PinOp{i + 1, 0}).ok());
    }
    assert(!pins.pin(PinOp{kPinRegistryCap + 1, 0}).ok());
    pins.shutdown();
}

void test_pin_alias() {
    using namespace memforge;
    PinRegistry pins;
    pins.init();
    pins.pin(PinOp{1, 0});
    assert(pins.pin(PinOp{2, 1}).ok());
    assert(pins.size() == 2);
    pins.shutdown();
}

void test_pin_release_idempotent() {
    using namespace memforge;
    PinRegistry pins;
    pins.init();
    assert(pins.releaseAll().ok());
    assert(pins.releaseAll().ok());
    pins.shutdown();
}

void test_pin_apply() {
    using namespace memforge;
    OpStream ops;
    ops.append(OpKind::Pin, 1, 0, 0, 0);
    ops.append(OpKind::PinReleaseAll, 0, 0, 0, 0);
    PinRegistry pins;
    pins.init();
    pins.apply(ops);
    assert(pins.size() == 0);
    pins.shutdown();
}
}  // namespace

// static memforge_test::Register r1("pin.release", &test_pin_release);
// static memforge_test::Register r2("pin.capacity", &test_pin_capacity);
// static memforge_test::Register r3("pin.alias", &test_pin_alias);
// static memforge_test::Register r4("pin.release_idempotent", &test_pin_release_idempotent);
// static memforge_test::Register r5("pin.apply", &test_pin_apply);
