// Example: pin registry alias demo.
//
// Demonstrates how aliasing pins share a payload and how releaseAll walks
// the registry. Useful for documenting the alias semantics.

#include <cstdio>

#include <memforge/api.hpp>

int main() {
    using namespace memforge;
    PinRegistry pins;
    pins.init();
    pins.pin(PinOp{1, 0});
    pins.pin(PinOp{2, 1});  // alias
    pins.pin(PinOp{3, 0});
    std::printf("pins: %zu\n", pins.size());

    if (pins.releaseAll().ok()) {
        std::printf("release ok\n");
    } else {
        std::printf("release failed\n");
    }
    pins.shutdown();
    return 0;
}
