#!/bin/bash -eu
# MemForge - harness build verification script.
#
# Mirrors the ClusterFuzzLite build but skips LIB_FUZZING_ENGINE linkage so it
# can compile cleanly on a workstation for smoke tests. Each harness is
# compiled with -fsanitize=fuzzer and the production object set.

set -o pipefail

cd "$(dirname "$0")/.."

OUT_DIR="fuzz/.verify-build"
mkdir -p "$OUT_DIR"

CXX=${CXX:-clang++}
CXXFLAGS=${CXXFLAGS:--fsanitize=address,undefined,fuzzer -g -O1 -fno-omit-frame-pointer}

PROD_SOURCES=$(find src -type f -name '*.cpp' | sort)
PROD_OBJECTS=()
for src in $PROD_SOURCES; do
    obj="$OUT_DIR/$(echo "$src" | tr '/' '_').o"
    "$CXX" $CXXFLAGS -Iinclude -Isrc -c "$src" -o "$obj"
    PROD_OBJECTS+=("$obj")
done

mapfile -t TARGETS < fuzz/targets.list

FAIL=0
for target in "${TARGETS[@]}"; do
    harness="fuzz/${target}.cc"
    if [[ ! -f "$harness" ]]; then
        echo "MISSING $harness"
        FAIL=1
        continue
    fi
    if ! "$CXX" $CXXFLAGS -Iinclude -Isrc -c "$harness" -o "$OUT_DIR/${target}.o" 2>"$OUT_DIR/${target}.log"; then
        echo "FAIL compile $target"
        FAIL=1
        continue
    fi
    if ! "$CXX" $CXXFLAGS "$OUT_DIR/${target}.o" "${PROD_OBJECTS[@]}" -o "$OUT_DIR/$target" 2>>"$OUT_DIR/${target}.log"; then
        echo "FAIL link $target"
        FAIL=1
        continue
    fi
    echo "OK   $target"
done

if [[ $FAIL -ne 0 ]]; then
    exit 1
fi
echo "All ${#TARGETS[@]} harnesses compiled."
