#!/bin/bash -eu
# MemForge - ClusterFuzzLite build script.
#
# Builds every harness in fuzz/targets.list into $OUT and copies the seed
# corpus / dictionary. The build is deterministic and self-contained.
#
# IMPORTANT: fuzzer binaries do NOT link src/cli/memforge.cpp (which defines
# `main()`). Only production sources under src/ (excluding src/cli/) are
# compiled into fuzzer object files.

set -o pipefail

cd "${SRC:-/src/memforge}"

OUT_DIR="${OUT:-/out}"
mkdir -p "$OUT_DIR"

# Production object set (shared by library). src/cli/ is excluded here.
PROD_SOURCES=$(find src -type f -name '*.cpp' -not -path 'src/cli/*' | sort)
PROD_OBJECTS=()
for src in $PROD_SOURCES; do
    obj="$OUT_DIR/$(echo "$src" | tr '/' '_').o"
    ${CXX:-clang++} ${CXXFLAGS:--fsanitize=address,undefined,fuzzer-no-link -g -O1} \
        -Iinclude -Isrc -c "$src" -o "$obj"
    PROD_OBJECTS+=("$obj")
done

# Targets list is the single source of truth.
mapfile -t TARGETS < fuzz/targets.list

for target in "${TARGETS[@]}"; do
    harness="fuzz/${target}.cc"
    if [[ ! -f "$harness" ]]; then
        echo "missing harness source: $harness" >&2
        exit 1
    fi
    ${CXX:-clang++} ${CXXFLAGS:--fsanitize=address,undefined,fuzzer-no-link -g -O1} \
        -Iinclude -Isrc -c "$harness" -o "$OUT_DIR/${target}.o"
    ${CXX:-clang++} ${CXXFLAGS:--fsanitize=address,undefined,fuzzer -g -O1} \
        "$OUT_DIR/${target}.o" "${PROD_OBJECTS[@]}" \
        ${LIB_FUZZING_ENGINE:--fsanitize=fuzzer} \
        -o "$OUT_DIR/$target"
done

# Copy the dictionary and per-harness seed corpora.
cp fuzz/dictionary.txt "$OUT_DIR/"

for target in "${TARGETS[@]}"; do
    corpus_dir="fuzz/corpus/$target"
    if [[ -d "$corpus_dir" ]]; then
        (cd "$corpus_dir" && zip -q -r "$OUT_DIR/${target}_seed_corpus.zip" .) || true
    fi
done

echo "memforge: built ${#TARGETS[@]} harnesses into $OUT_DIR"
