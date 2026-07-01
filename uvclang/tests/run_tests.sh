#!/bin/sh
#
# Differential test harness for uvclang.
#
# For each tests/*.c, at -O0, -O1 and -O2:
#   1. Compile natively (reference) and run     -> exit code + stdout
#   2. Generate .ll (gen_ll.sh), compile via uvclang -> .asm
#   3. Run the .asm in UVM                       -> exit code + stdout
#   4. Compare. UVM exits with main's return as the process code, same as
#      native (both truncated to 8 bits), so the codes compare directly.
#
# Tests uvclang cannot compile yet are reported as SKIP, not FAIL.
#
# Env overrides: NATIVE_CC (default cc), CLANG (used by gen_ll.sh).

set -u

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
UVCLANG="$ROOT/uvclang"
VM="$ROOT/vm"
TESTS="$UVCLANG/tests"
NATIVE_CC="${NATIVE_CC:-cc}"

# Build uvclang and uvm once.
( cd "$UVCLANG" && cargo build -q ) || { echo "uvclang build failed"; exit 1; }
( cd "$VM" && cargo build -q ) || { echo "uvm build failed"; exit 1; }
UVCLANG_BIN="$UVCLANG/target/debug/uvclang"
UVM_BIN="$VM/target/debug/uvm"

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

pass=0; fail=0; skip=0

# Exercise each test at multiple optimization levels: -O0/-O1/-O2 produce
# different IR shapes (e.g. -O0 keeps everything in allocas/loads/stores rather
# than SSA registers) and so hit different uvclang code paths.
OPT_LEVELS="-O0 -O1 -O2"

for src in "$TESTS"/*.c; do
    base=$(basename "$src" .c)
    for opt in $OPT_LEVELS; do
        name="$base ($opt)"
        ll="$TMP/$base$opt.ll"

        # Generate the .ll fresh into the temp dir (it is a build artifact, not
        # checked in). A clang failure means we can't produce IR for this test.
        if ! OPT="$opt" "$TESTS/gen_ll.sh" "$src" >"$ll" 2>/dev/null; then
            echo "SKIP $name (no .ll)"; skip=$((skip+1)); continue
        fi

        # Native reference, built at the same optimization level as the IR.
        # Note: no -I here on purpose -- the reference must use the platform's
        # own libc and clang's headers, never uvclang's UVM-side headers, so that
        # stdlib headers are genuinely tested differentially.
        if ! "$NATIVE_CC" "$opt" -w "$src" -o "$TMP/ref" 2>/dev/null; then
            echo "SKIP $name (native compile failed)"; skip=$((skip+1)); continue
        fi
        ref_out=$("$TMP/ref" 2>/dev/null); ref_code=$?

        # Compile via uvclang.
        if ! "$UVCLANG_BIN" "$ll" -o "$TMP/out.asm" 2>"$TMP/err"; then
            echo "SKIP $name (uvclang: $(head -1 "$TMP/err"))"; skip=$((skip+1)); continue
        fi

        # Run in UVM.
        uvm_out=$("$UVM_BIN" "$TMP/out.asm" 2>/dev/null); uvm_code=$?

        if [ "$ref_code" = "$uvm_code" ] && [ "$ref_out" = "$uvm_out" ]; then
            echo "PASS $name (exit=$ref_code)"; pass=$((pass+1))
        else
            echo "FAIL $name: native(exit=$ref_code, out='$ref_out') vs uvm(exit=$uvm_code, out='$uvm_out')"
            fail=$((fail+1))
        fi
    done
done

echo "--------"
echo "pass=$pass fail=$fail skip=$skip"
[ "$fail" -eq 0 ]
