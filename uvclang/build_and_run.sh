#!/bin/sh
# Compile a source file with uvclang and run the result in the UVM VM.
# All arguments are forwarded to uvclang, so flags work as usual:
#   ./build_and_run.sh examples/synth.c
#   ./build_and_run.sh -O0 tests/printf.c
set -e
cd "$(dirname "$0")"
export RUST_BACKTRACE=1
cargo run -q -- "$@" -o out.asm
exec cargo run -q --manifest-path ../vm/Cargo.toml -- out.asm
