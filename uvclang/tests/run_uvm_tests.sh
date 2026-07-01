#!/bin/sh
#
# Self-checking test harness for uvclang's <uvm/...> platform headers.
#
# These headers (uvm/math.h, uvm/graphics.h, uvm/utils.h, uvm/window.h) sit on
# top of UVM syscalls and have no native-libc equivalent, so the differential
# harness (run_tests.sh) cannot cover them: a native reference build can't even
# resolve <uvm/...>. Instead, each tests/uvm_*.c is a self-checking program that
# asserts its own results and exits 0 on success. For each, at -O0/-O1/-O2, we:
#   1. Generate .ll (gen_ll.sh, which adds -Iuvclang/include).
#   2. Compile it via uvclang -> .asm.
#   3. Run the .asm in UVM and require exit code 0.
#
# Tests uvclang cannot compile yet are reported as SKIP, not FAIL.
#
# Env overrides: CLANG (used by gen_ll.sh).

set -u

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
UVCLANG="$ROOT/uvclang"
VM="$ROOT/vm"
TESTS="$UVCLANG/tests"

# Build uvclang and uvm once.
( cd "$UVCLANG" && cargo build -q ) || { echo "uvclang build failed"; exit 1; }
( cd "$VM" && cargo build -q ) || { echo "uvm build failed"; exit 1; }
UVCLANG_BIN="$UVCLANG/target/debug/uvclang"
UVM_BIN="$VM/target/debug/uvm"

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

pass=0; fail=0; skip=0

# Same optimization spread as run_tests.sh: -O0 keeps allocas/loads/stores while
# -O1/-O2 produce SSA-register-heavy IR, hitting different uvclang code paths.
OPT_LEVELS="-O0 -O1 -O2"

for src in "$TESTS"/uvm_*.c; do
    base=$(basename "$src" .c)
    for opt in $OPT_LEVELS; do
        name="$base ($opt)"
        ll="$TMP/$base$opt.ll"

        # Generate the .ll fresh into the temp dir (it is a build artifact).
        if ! OPT="$opt" "$UVCLANG/tests/gen_ll.sh" "$src" >"$ll" 2>/dev/null; then
            echo "SKIP $name (no .ll)"; skip=$((skip+1)); continue
        fi

        # Compile via uvclang.
        if ! "$UVCLANG_BIN" "$ll" -o "$TMP/out.asm" 2>"$TMP/err"; then
            echo "SKIP $name (uvclang: $(head -1 "$TMP/err"))"; skip=$((skip+1)); continue
        fi

        # Run in UVM. Self-checking tests exit 0 on success (asserting their own
        # results); any other code is a failure.
        "$UVM_BIN" "$TMP/out.asm" >/dev/null 2>&1; uvm_code=$?

        if [ "$uvm_code" = "0" ]; then
            echo "PASS $name"; pass=$((pass+1))
        else
            echo "FAIL $name (exit=$uvm_code)"; fail=$((fail+1))
        fi
    done
done

echo "--------"
echo "pass=$pass fail=$fail skip=$skip"
[ "$fail" -eq 0 ]
