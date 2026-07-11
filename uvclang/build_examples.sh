#!/bin/sh
#
# Compile-and-parse smoke test for uvclang/examples/*.c.
#
# The examples are whole programs (a subtractive synth, an LZ77 compressor,
# ...). Several are graphical or audio programs that can't run headless in CI,
# so unlike the tests/ harnesses this one does NOT execute them. Instead, for
# each examples/*.c at -O0/-O1/-O2 it:
#   1. Compiles the source through uvclang -> .asm (the front end drives clang
#      in-process, exactly as `uvclang foo.c` does).
#   2. Runs the VM in `--parse-only` mode over the .asm, which parses/validates
#      the assembly without running it, and requires success.
#
# Unlike run_tests.sh / run_uvm_tests.sh, a failure here is a FAIL, not a
# SKIP: the examples are curated, working programs, so a compile or parse
# regression in any of them should break the build.
#
# Env overrides: CLANG/CLANGXX (used by the uvclang front end).

set -u

cd "$(dirname "$0")"
UVCLANG=$(pwd)
ROOT=$(cd .. && pwd)
VM="$ROOT/vm"
EXAMPLES="$UVCLANG/examples"

# Build uvclang and uvm once.
( cd "$UVCLANG" && cargo build -q ) || { echo "uvclang build failed"; exit 1; }
( cd "$VM" && cargo build -q ) || { echo "uvm build failed"; exit 1; }
UVCLANG_BIN="$UVCLANG/target/debug/uvclang"
UVM_BIN="$VM/target/debug/uvm"

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

pass=0; fail=0

# Same optimization spread as the tests/ harnesses: -O0 keeps allocas/loads/
# stores while -O1/-O2 produce SSA-register-heavy IR, hitting different uvclang
# code paths, so each example is exercised through all three.
OPT_LEVELS="-O0 -O1 -O2"

for src in "$EXAMPLES"/*.c; do
    base=$(basename "$src" .c)
    for opt in $OPT_LEVELS; do
        name="$base ($opt)"

        # One command: C source straight to UVM asm (clang driven in-process).
        if ! "$UVCLANG_BIN" "$opt" "$src" -o "$TMP/out.asm" 2>"$TMP/err"; then
            echo "FAIL $name (uvclang: $(head -1 "$TMP/err"))"; fail=$((fail+1)); continue
        fi

        # Parse/validate the asm without running it. Parse errors are printed on
        # stdout, so capture both streams for the diagnostic.
        if ! "$UVM_BIN" --parse-only "$TMP/out.asm" >"$TMP/perr" 2>&1; then
            echo "FAIL $name (uvm parse: $(head -1 "$TMP/perr"))"; fail=$((fail+1)); continue
        fi

        echo "PASS $name"; pass=$((pass+1))
    done
done

echo "--------"
echo "pass=$pass fail=$fail"
[ "$fail" -eq 0 ]
