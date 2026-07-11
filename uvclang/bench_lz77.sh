#!/bin/sh
#
# bench_lz77.sh — benchmark examples/lz77.c three ways and confirm they all
# compute the same result:
#
#   1. UVM (release)             : uvclang -O2 lowers the C to UVM asm, run in
#                                  the release UVM interpreter.
#   2. native -O2, SIMD on       : host clang at -O2 with the auto-vectorizers
#                                  enabled (a normal optimized native build).
#   3. native -O2, SIMD off      : host clang at -O2 with -fno-vectorize
#                                  -fno-slp-vectorize, i.e. scalar codegen.
#
# (2) vs (3) shows how much auto-vectorization buys on native; (3) is the
# closest apples-to-apples baseline for the scalar UVM interpreter, so times are
# normalized against it. All three print the same checksum line, which the
# script cross-checks so a silent divergence can't masquerade as a speedup.
#
# Env overrides:
#   CC           native C compiler (default: Homebrew LLVM clang, else cc)
#   UVM_REPS     timed repetitions for the UVM run    (default 3)
#   NATIVE_REPS  timed repetitions for each native run (default 5)
#
# Timing is best-of-N wall clock via python3's perf_counter (sub-millisecond).

set -eu
cd "$(dirname "$0")"
ROOT=$(cd .. && pwd)
SRC=examples/lz77.c

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# Resolve the native compiler: explicit $CC, else Homebrew LLVM clang (same
# compiler the uvclang front-end uses, so native and UVM share a front end),
# else the platform cc.
if [ -n "${CC:-}" ]; then
    :
elif [ -x /opt/homebrew/opt/llvm/bin/clang ]; then
    CC=/opt/homebrew/opt/llvm/bin/clang
else
    CC=cc
fi

echo "native compiler : $CC"

echo "building uvclang (release)..."
cargo build --release -q
UVCLANG="$ROOT/uvclang/target/release/uvclang"

echo "building UVM (release)..."
cargo build --release -q --manifest-path "$ROOT/vm/Cargo.toml"
UVM="$ROOT/vm/target/release/uvm"

echo "compiling $SRC ..."
# UVM build: uvclang drives clang for the x86_64 UVM target (scalar; its
# canonical flags already include -fno-vectorize) and produces UVM asm.
"$UVCLANG" -O2 "$SRC" -o "$TMP/lz77.asm"
# Native builds: host target, SIMD on and SIMD off.
"$CC" -O2 "$SRC" -o "$TMP/native_simd"
"$CC" -O2 -fno-vectorize -fno-slp-vectorize "$SRC" -o "$TMP/native_nosimd"

export UVM ASM="$TMP/lz77.asm"
export NAT_SIMD="$TMP/native_simd" NAT_NOSIMD="$TMP/native_nosimd"
export UVM_REPS="${UVM_REPS:-3}" NATIVE_REPS="${NATIVE_REPS:-5}"

python3 - <<'PY'
import os, subprocess, sys, time

def bench(label, reps, cmd):
    best = None
    out = ""
    for _ in range(reps):
        t0 = time.perf_counter()
        r = subprocess.run(cmd, capture_output=True, text=True)
        dt = time.perf_counter() - t0
        if r.returncode != 0:
            print(f"\n{label}: FAILED (exit {r.returncode})\n{r.stderr}", file=sys.stderr)
            sys.exit(1)
        out = r.stdout
        best = dt if best is None else min(best, dt)
    return best, out

configs = [
    ("UVM (release)",        int(os.environ["UVM_REPS"]),    [os.environ["UVM"], os.environ["ASM"]]),
    ("native -O2, SIMD on",  int(os.environ["NATIVE_REPS"]), [os.environ["NAT_SIMD"]]),
    ("native -O2, SIMD off", int(os.environ["NATIVE_REPS"]), [os.environ["NAT_NOSIMD"]]),
]

results = [(label, *bench(label, reps, cmd), reps) for label, reps, cmd in configs]

def checksum(out):
    for line in out.splitlines():
        if "checksum=" in line:
            return line.split("checksum=", 1)[1].strip()
    return None

baseline_csum = checksum(results[0][2])
parity = all(checksum(out) == baseline_csum for _, _, out, _ in results)

# Normalize against native SIMD-off (the scalar apples-to-apples baseline).
base_t = next(t for label, t, _, _ in results if label == "native -O2, SIMD off")

print()
print("== lz77 benchmark ==")
for line in results[0][2].splitlines():   # the program's own summary (identical for all)
    print("  " + line)
print()
print(f"  {'configuration':24s} {'best time':>11s}   {'vs SIMD-off':>11s}")
print("  " + "-" * 54)
for label, t, _, reps in results:
    print(f"  {label:24s} {t*1000:8.2f} ms   {t/base_t:9.2f}x  (best of {reps})")
print()

if parity:
    print("  checksums MATCH across all three builds")
else:
    print("  checksums DIFFER -- builds disagree on the result:")
    for label, _, out, _ in results:
        print(f"    {label:24s} {checksum(out)}")
    sys.exit(1)
PY
