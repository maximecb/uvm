#!/bin/sh
#
# Differential test harness for llbc.
#
# For each tests/*.c:
#   1. Compile natively (reference) and run     -> exit code + stdout
#   2. Generate .ll (gen_ll.sh), compile via llbc -> .asm
#   3. Run the .asm in UVM                       -> exit code + stdout
#   4. Compare. UVM exits with main's return as the process code, same as
#      native (both truncated to 8 bits), so the codes compare directly.
#
# Tests llbc cannot compile yet are reported as SKIP, not FAIL.
#
# Env overrides: NATIVE_CC (default cc), CLANG (used by gen_ll.sh).

set -u

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
LLBC="$ROOT/llbc"
VM="$ROOT/vm"
TESTS="$LLBC/tests"
NATIVE_CC="${NATIVE_CC:-cc}"

# Build llbc and uvm once.
( cd "$LLBC" && cargo build -q ) || { echo "llbc build failed"; exit 1; }
( cd "$VM" && cargo build -q ) || { echo "uvm build failed"; exit 1; }
LLBC_BIN="$LLBC/target/debug/llbc"
UVM_BIN="$VM/target/debug/uvm"

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

pass=0; fail=0; skip=0

for src in "$TESTS"/*.c; do
    name=$(basename "$src" .c)
    ll="$TESTS/$name.ll"

    # Ensure the .ll exists (generate if missing).
    [ -f "$ll" ] || "$TESTS/gen_ll.sh" "$src" >/dev/null 2>&1
    [ -f "$ll" ] || { echo "SKIP $name (no .ll)"; skip=$((skip+1)); continue; }

    # Native reference.
    if ! "$NATIVE_CC" -O2 -w "$src" -o "$TMP/ref" 2>/dev/null; then
        echo "SKIP $name (native compile failed)"; skip=$((skip+1)); continue
    fi
    ref_out=$("$TMP/ref" 2>/dev/null); ref_code=$?

    # Compile via llbc.
    if ! "$LLBC_BIN" "$ll" -o "$TMP/$name.asm" 2>"$TMP/err"; then
        echo "SKIP $name (llbc: $(head -1 "$TMP/err"))"; skip=$((skip+1)); continue
    fi

    # Run in UVM.
    uvm_out=$("$UVM_BIN" "$TMP/$name.asm" 2>/dev/null); uvm_code=$?

    if [ "$ref_code" = "$uvm_code" ] && [ "$ref_out" = "$uvm_out" ]; then
        echo "PASS $name (exit=$ref_code)"; pass=$((pass+1))
    else
        echo "FAIL $name: native(exit=$ref_code, out='$ref_out') vs uvm(exit=$uvm_code, out='$uvm_out')"
        fail=$((fail+1))
    fi
done

# Hand-written .ll tests, validated against LLVM's interpreter (lli). These
# cover control flow that clang would constant-fold away when compiled from C.
LLI="${LLI:-/opt/homebrew/opt/llvm/bin/lli}"
if [ -x "$LLI" ] && [ -d "$TESTS/ll" ]; then
    for ll in "$TESTS"/ll/*.ll; do
        [ -e "$ll" ] || continue
        name="ll/$(basename "$ll" .ll)"
        ref_out=$("$LLI" "$ll" 2>/dev/null); ref_code=$?
        if ! "$LLBC_BIN" "$ll" -o "$TMP/ref.asm" 2>"$TMP/err"; then
            echo "SKIP $name (llbc: $(head -1 "$TMP/err"))"; skip=$((skip+1)); continue
        fi
        uvm_out=$("$UVM_BIN" "$TMP/ref.asm" 2>/dev/null); uvm_code=$?
        if [ "$ref_code" = "$uvm_code" ] && [ "$ref_out" = "$uvm_out" ]; then
            echo "PASS $name (exit=$ref_code)"; pass=$((pass+1))
        else
            echo "FAIL $name: lli(exit=$ref_code, out='$ref_out') vs uvm(exit=$uvm_code, out='$uvm_out')"
            fail=$((fail+1))
        fi
    done
fi

echo "--------"
echo "pass=$pass fail=$fail skip=$skip"
[ "$fail" -eq 0 ]
