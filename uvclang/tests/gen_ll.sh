#!/bin/sh
# Emit textual LLVM IR (.ll) on stdout for a C source file, using the canonical
# flags for the uvclang front-end. The optimization level defaults to -O2; override
# it with OPT (e.g. OPT=-O1). The IR is a build artifact and is not checked in:
# pipe it into uvclang, or redirect to a scratch file for inspection.
#
# Usage: ./gen_ll.sh foo.c                # -O2 IR to stdout
#        OPT=-O1 ./gen_ll.sh foo.c         # -O1 IR to stdout
#        ./gen_ll.sh foo.c > /tmp/foo.ll   # or redirect for inspection
# Resolve clang: honor $CLANG, else prefer Homebrew LLVM (a common macOS dev
# setup), else fall back to whatever `clang` is on PATH (Linux / CI).
if [ -z "${CLANG:-}" ]; then
    if [ -x /opt/homebrew/opt/llvm/bin/clang ]; then
        CLANG=/opt/homebrew/opt/llvm/bin/clang
    else
        CLANG=clang
    fi
fi
OPT="${OPT:--O2}"
# UVM include dir: uvclang's own headers (<uvm/syscalls.h>, <string.h>, ...) live
# here. This is the UVM build ONLY. The native reference build in run_tests.sh
# never adds this -I, so it uses the platform's own libc + clang's headers, and
# stdlib headers are checked differentially (uvclang's impl vs the system libc).
INCDIR="$(cd "$(dirname "$0")/../include" && pwd)"
exec "$CLANG" \
    --target=x86_64-unknown-linux-gnu \
    "$OPT" \
    -I"$INCDIR" \
    -fno-discard-value-names \
    -fno-optimize-sibling-calls \
    -fno-vectorize \
    -fno-slp-vectorize \
    -fno-jump-tables \
    -fno-strict-aliasing \
    -S -emit-llvm \
    "$1" -o -
