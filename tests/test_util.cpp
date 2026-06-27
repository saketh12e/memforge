#include <cassert>
#include <cstdio>
#include <vector>

#include "test_framework.hpp"
#include <memforge/api.hpp>
#include <memforge/align.hpp>
#include <memforge/hash.hpp>
#include <memforge/bits.hpp>

// Tests for the small utility headers.
namespace {
void test_align_up() {
    using namespace memforge;
    assert(align_up(0, 8) == 0);
    assert(align_up(1, 8) == 8);
    assert(align_up(7, 8) == 8);
    assert(align_up(8, 8) == 8);
    assert(align_up(9, 8) == 16);
    assert(align_up(16, 16) == 16);
}

void test_align_down() {
    using namespace memforge;
    assert(align_down(0, 8) == 0);
    assert(align_down(7, 8) == 0);
    assert(align_down(8, 8) == 8);
    assert(align_down(15, 8) == 8);
}

void test_is_power_of_two() {
    using namespace memforge;
    assert(!is_power_of_two(0));
    assert(is_power_of_two(1));
    assert(is_power_of_two(2));
    assert(!is_power_of_two(3));
    assert(is_power_of_two(4));
    assert(!is_power_of_two(7));
    assert(is_power_of_two(1024));
}

void test_next_power_of_two() {
    using namespace memforge;
    assert(next_power_of_two(1) == 1);
    assert(next_power_of_two(2) == 2);
    assert(next_power_of_two(3) == 4);
    assert(next_power_of_two(5) == 8);
    assert(next_power_of_two(1023) == 1024);
}

void test_fnv1a_32() {
    using namespace memforge;
    std::uint8_t bytes[] = {'h', 'e', 'l', 'l', 'o'};
    std::uint32_t h = fnv1a_32(bytes, 5);
    assert(h != 0);
    assert(fnv1a_32(bytes, 5) == h);  // deterministic
}

void test_knuth_hash() {
    using namespace memforge;
    assert(knuth_hash(0) == 0);
    assert(knuth_hash(1) != knuth_hash(2));
}

void test_popcount32() {
    using namespace memforge;
    assert(popcount32(0) == 0);
    assert(popcount32(1) == 1);
    assert(popcount32(3) == 2);
    assert(popcount32(0xFF) == 8);
    assert(popcount32(0xFFFFFFFFu) == 32);
}

void test_clz32() {
    using namespace memforge;
    assert(clz32(0) == 32);
    assert(clz32(1) == 31);
    assert(clz32(0x80000000u) == 0);
}

void test_ctz32() {
    using namespace memforge;
    assert(ctz32(0) == 32);
    assert(ctz32(1) == 0);
    assert(ctz32(2) == 1);
    assert(ctz32(8) == 3);
}

void test_bswap32() {
    using namespace memforge;
    assert(bswap32(0x12345678u) == 0x78563412u);
    assert(bswap32(0xAABBCCDDu) == 0xDDCCBBAAu);
}

void test_rotl_rotr() {
    using namespace memforge;
    assert(rotl32(0x00000001u, 1) == 0x00000002u);
    assert(rotr32(0x00000002u, 1) == 0x00000001u);
}

void test_mix_slot() {
    using namespace memforge;
    assert(mix_slot(1, 0) != mix_slot(2, 0));
    assert(mix_slot(1, 0) == mix_slot(1, 0));  // deterministic
}
}  // namespace

static memforge_test::Register r1("util.align_up", &test_align_up);
static memforge_test::Register r2("util.align_down", &test_align_down);
static memforge_test::Register r3("util.is_power_of_two", &test_is_power_of_two);
static memforge_test::Register r4("util.next_power_of_two", &test_next_power_of_two);
static memforge_test::Register r5("util.fnv1a_32", &test_fnv1a_32);
static memforge_test::Register r6("util.knuth_hash", &test_knuth_hash);
static memforge_test::Register r7("util.popcount32", &test_popcount32);
static memforge_test::Register r8("util.clz32", &test_clz32);
static memforge_test::Register r9("util.ctz32", &test_ctz32);
static memforge_test::Register r10("util.bswap32", &test_bswap32);
static memforge_test::Register r11("util.rotl_rotr", &test_rotl_rotr);
static memforge_test::Register r12("util.mix_slot", &test_mix_slot);
