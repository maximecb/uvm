#!/bin/sh
# Emit textual LLVM IR (.ll) on stdout for a C source file, using the canonical
# flags for the llbc front-end. The optimization level defaults to -O2; override
# it with OPT (e.g. OPT=-O1). The IR is a build artifact and is not checked in:
# pipe it into llbc, or redirect to a scratch file for inspection.
#
# Usage: ./gen_ll.sh foo.c                # -O2 IR to stdout
#        OPT=-O1 ./gen_ll.sh foo.c         # -O1 IR to stdout
#        ./gen_ll.sh foo.c > /tmp/foo.ll   # or redirect for inspection
CLANG="${CLANG:-/opt/homebrew/opt/llvm/bin/clang}"
OPT="${OPT:--O2}"
exec "$CLANG" \
    --target=x86_64-unknown-linux-gnu \
    "$OPT" \
    -fno-discard-value-names \
    -fno-optimize-sibling-calls \
    -fno-vectorize \
    -fno-slp-vectorize \
    -fno-jump-tables \
    -fno-strict-aliasing \
    -S -emit-llvm \
    "$1" -o -
