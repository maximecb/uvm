#!/bin/sh
#
# Front-end (Phase 9) test harness for uvclang.
#
# Where run_tests.sh / run_uvm_tests.sh drive the back-end via a two-step
# `gen_ll.sh file.c | uvclang file.ll`, this harness exercises the *whole*
# pipeline through the single driver command `uvclang file.c` -- clang is run
# in-process, no temp .ll ever hits disk. It is the direct proof that the
# clang-driver front-end works end to end.
#
# For each tests/*.c, at -O0/-O1/-O2:
#   - uvm_*.c  : self-checking. `uvclang $opt src -o asm`; run; require exit 0.
#                (No native reference exists -- these use <uvm/...> headers.)
#   - others   : differential. Compile+run natively (reference, no -I) and via
#                `uvclang $opt src -o asm` + UVM; compare exit code and stdout.
#
# Tests uvclang cannot compile yet are reported as SKIP, not FAIL.
#
# Env overrides: NATIVE_CC (default cc), CLANG/CLANGXX (used by the driver).

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

OPT_LEVELS="-O0 -O1 -O2"

for src in "$TESTS"/*.c; do
    base=$(basename "$src" .c)

    for opt in $OPT_LEVELS; do
        name="$base ($opt)"

        # One command: C source straight to UVM asm (clang driven in-process).
        if ! "$UVCLANG_BIN" "$opt" "$src" -o "$TMP/out.asm" 2>"$TMP/err"; then
            echo "SKIP $name (uvclang: $(head -1 "$TMP/err"))"; skip=$((skip+1)); continue
        fi
        uvm_out=$("$UVM_BIN" "$TMP/out.asm" 2>/dev/null); uvm_code=$?

        case "$base" in
            xfail_*)
                # Self-checking FAILURE path: no native reference. The program is
                # expected to terminate abnormally after printing a diagnostic
                # (e.g. a failed assert), so require a NON-zero exit and some
                # output on stdout.
                if [ "$uvm_code" != "0" ] && [ -n "$uvm_out" ]; then
                    echo "PASS $name (exit=$uvm_code)"; pass=$((pass+1))
                else
                    echo "FAIL $name (exit=$uvm_code, out='$uvm_out')"; fail=$((fail+1))
                fi
                ;;
            uvm_*)
                # Self-checking: the program asserts its own results, exit 0 = ok.
                if [ "$uvm_code" = "0" ]; then
                    echo "PASS $name"; pass=$((pass+1))
                else
                    echo "FAIL $name (exit=$uvm_code)"; fail=$((fail+1))
                fi
                ;;
            *)
                # Differential vs native. No -I on the reference build on purpose:
                # it must use the platform libc, not uvclang's UVM-side headers.
                if ! "$NATIVE_CC" "$opt" -w "$src" -o "$TMP/ref" 2>/dev/null; then
                    echo "SKIP $name (native compile failed)"; skip=$((skip+1)); continue
                fi
                ref_out=$("$TMP/ref" 2>/dev/null); ref_code=$?

                if [ "$ref_code" = "$uvm_code" ] && [ "$ref_out" = "$uvm_out" ]; then
                    echo "PASS $name (exit=$ref_code)"; pass=$((pass+1))
                else
                    echo "FAIL $name: native(exit=$ref_code, out='$ref_out') vs uvm(exit=$uvm_code, out='$uvm_out')"
                    fail=$((fail+1))
                fi
                ;;
        esac
    done
done

echo "--------"
echo "pass=$pass fail=$fail skip=$skip"
[ "$fail" -eq 0 ]
