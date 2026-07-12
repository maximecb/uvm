#!/bin/sh
# Compile a source file with uvclang and run the result in the UVM VM.
# All arguments are forwarded to uvclang, so flags work as usual. Works from any
# directory; input paths resolve against your current directory:
#   ./build_and_run.sh examples/synth.c
#   uvclang/build_and_run.sh uvclang/tests/printf.c   # e.g. from the repo root
set -e

# Resolve this script's directory so we can find the crates and write out.asm
# here regardless of the caller's working directory. We deliberately do NOT cd:
# staying in the caller's directory lets relative input paths resolve naturally
# (uvclang's own include dir is compiled-in absolute, so it needs no cd).
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

export RUST_BACKTRACE=1
cargo run -q --manifest-path "$SCRIPT_DIR/Cargo.toml" -- "$@" -o "$SCRIPT_DIR/out.asm"
exec cargo run -q --manifest-path "$SCRIPT_DIR/../vm/Cargo.toml" -- "$SCRIPT_DIR/out.asm"
