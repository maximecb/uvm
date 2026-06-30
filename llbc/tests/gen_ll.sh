#!/bin/sh
# Generate textual LLVM IR (.ll) from a C test file, using the canonical
# flags for the llbc front-end. Output goes next to the input as <name>.ll.
#
# Usage: ./gen_ll.sh foo.c [bar.c ...]
CLANG="${CLANG:-/opt/homebrew/opt/llvm/bin/clang}"
for src in "$@"; do
    out="${src%.c}.ll"
    "$CLANG" \
        --target=x86_64-unknown-linux-gnu \
        -O2 \
        -fno-discard-value-names \
        -fno-optimize-sibling-calls \
        -fno-vectorize \
        -fno-slp-vectorize \
        -fno-jump-tables \
        -fno-strict-aliasing \
        -S -emit-llvm \
        "$src" -o "$out" && echo "generated $out"
done
