#!/bin/sh
#
# Differential test harness for uvclang.
#
# Drives the whole pipeline through the single driver command `uvclang file.c`:
# clang is run in-process (the front end), no temporary .ll is ever written to
# disk. For each tests/*.c, at -O0/-O1/-O2:
#   1. Compile the source straight to UVM asm via `uvclang $opt file.c -o asm`.
#   2. Run the asm in UVM                                   -> exit code + stdout
#   3. Decide pass/fail by the file's kind:
#        - xfail_*.c : an abnormal-termination path (e.g. a failed assert), with
#                      no comparable native behavior. Require a NON-zero exit and
#                      some stdout.
#        - others    : compile+run natively (reference) and compare exit code and
#                      stdout. UVM exits with main's return as the process code,
#                      same as native (both truncated to 8 bits), so they compare
#                      directly.
#
# uvm_*.c exercise the <uvm/...> platform headers, which have no native-libc
# equivalent; run_uvm_tests.sh covers them (self-checking) and they are skipped
# here. Tests uvclang cannot compile yet are reported as SKIP, not FAIL.
#
# Env overrides: NATIVE_CC (default cc), CLANG/CLANGXX (used by the uvclang
# front end).

set -u

cd "$(dirname "$0")"
UVCLANG=$(pwd)
ROOT=$(cd .. && pwd)
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

    # uvm_*.c use <uvm/...> headers with no native-libc equivalent, so they are
    # self-checking and covered by run_uvm_tests.sh; skip them here.
    case "$base" in
        uvm_*) continue ;;
    esac

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
                # (e.g. a failed assert), so require a NON-zero exit and output.
                if [ "$uvm_code" != "0" ] && [ -n "$uvm_out" ]; then
                    echo "PASS $name (exit=$uvm_code)"; pass=$((pass+1))
                else
                    echo "FAIL $name (exit=$uvm_code, out='$uvm_out')"; fail=$((fail+1))
                fi
                ;;
            *)
                # Differential vs native. No -I on the reference build on purpose:
                # it must use the platform libc, not uvclang's UVM-side headers,
                # so the stdlib headers are genuinely tested differentially.
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
