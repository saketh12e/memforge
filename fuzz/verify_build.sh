#!/bin/bash
# MemForge - harness build verification script.
#
# Mirrors the ClusterFuzzLite object set but links a small file-input driver so
# workstation smoke builds do not require a local libFuzzer runtime.

set -o pipefail

cd "$(dirname "$0")/.."

OUT_DIR="fuzz/.verify-build"
mkdir -p "$OUT_DIR"

CXX=${CXX:-clang++}
CXXFLAGS=${CXXFLAGS:--std=c++17 -fsanitize=address,undefined -g -O1 -fno-omit-frame-pointer}

cat >"$OUT_DIR/local_driver.cc" <<'DRIVER'
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size);

int main(int argc, char** argv) {
    if (argc < 2) {
        std::uint8_t empty = 0;
        return LLVMFuzzerTestOneInput(&empty, 0);
    }
    for (int i = 1; i < argc; ++i) {
        std::ifstream in(argv[i], std::ios::binary);
        if (!in) {
            std::fprintf(stderr, "cannot open %s\n", argv[i]);
            return 2;
        }
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                                        std::istreambuf_iterator<char>());
        const std::uint8_t* data = bytes.empty() ? nullptr : bytes.data();
        int rc = LLVMFuzzerTestOneInput(data, bytes.size());
        if (rc != 0) {
            return rc;
        }
    }
    return 0;
}
DRIVER
"$CXX" $CXXFLAGS -c "$OUT_DIR/local_driver.cc" -o "$OUT_DIR/local_driver.o"

# Exclude src/cli/* (defines main(); clashes with fuzzer main() driver).
PROD_SOURCES=$(find src -type f -name '*.cpp' ! -path 'src/cli/*' | sort)
PROD_OBJECTS=()
for src in $PROD_SOURCES; do
    obj="$OUT_DIR/$(echo "$src" | tr '/' '_').o"
    "$CXX" $CXXFLAGS -Iinclude -Isrc -c "$src" -o "$obj"
    PROD_OBJECTS+=("$obj")
done

# Portable target loading (no bash 4+ mapfile dependency).
TARGETS=()
while IFS= read -r target; do
    [ -n "$target" ] && TARGETS+=("$target")
done < fuzz/targets.list

if [ "${#TARGETS[@]}" -eq 0 ]; then
    echo "No targets found in fuzz/targets.list"
    exit 1
fi

FAIL=0
for target in "${TARGETS[@]}"; do
    harness="fuzz/${target}.cc"
    if [ ! -f "$harness" ]; then
        echo "MISSING $harness"
        FAIL=1
        continue
    fi
    if ! "$CXX" $CXXFLAGS -Iinclude -Isrc -c "$harness" -o "$OUT_DIR/${target}.o" 2>"$OUT_DIR/${target}.log"; then
        echo "FAIL compile $target"
        FAIL=1
        continue
    fi
    if ! "$CXX" $CXXFLAGS "$OUT_DIR/${target}.o" "$OUT_DIR/local_driver.o" "${PROD_OBJECTS[@]}" -o "$OUT_DIR/$target" 2>>"$OUT_DIR/${target}.log"; then
        echo "FAIL link $target"
        FAIL=1
        continue
    fi
    echo "OK   $target"
done

if [ "$FAIL" -ne 0 ]; then
    exit 1
fi
echo "All ${#TARGETS[@]} harnesses compiled."
