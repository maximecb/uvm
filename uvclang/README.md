# uvclang - a C compiler for UVM

`uvclang` compiles C source files into [UVM](../README.md) assembly. It uses
**clang as a front end**: it drives clang in-process to lower your C to textual
LLVM IR, then compiles that IR down to UVM assembly with its own back end. No
temporary `.ll` file is written to disk — a single `uvclang foo.c` command takes
you from C straight to a runnable `.asm`.

Because clang does the parsing, uvclang inherits the full C language (and, via
`clang++`, C++ — though C is the primary tested path today). The C standard
library and preprocessor, `stdint.h`, `_Bool`, real integer promotion rules,
`switch`, `goto`, unions, enums, variadic functions, and so on all just work,
because clang handles them before uvclang ever sees the code. The remaining
limitations live in the **LLVM IR → UVM back end**, not in a C parser.

## Requirements

- The [Rust toolchain](https://www.rust-lang.org/tools/install) (to build uvclang and UVM).
- A reasonably recent **clang / LLVM** installation, used as the front end.
  - On macOS, Homebrew LLVM (`brew install llvm`) is auto-detected at
    `/opt/homebrew/opt/llvm/bin/clang`.
  - On Linux / CI, whatever `clang` is on your `PATH` is used.
  - Override the binary with the `$CLANG` (or `$CLANGXX` for C++) environment variable.

## What the back end supports

Since clang handles the C language itself, the interesting question is what the
IR-to-UVM back end can lower. Currently that includes:

- Signed and unsigned integer arithmetic and bitwise ops, 32- and 64-bit
- `float` and `double` floating-point arithmetic
- Pointers, pointer arithmetic, and casts
- Global variables, arrays, N-dimensional arrays, and initializers
- Structs and unions (by value and by pointer), enums
- Functions, recursion, function pointers, and variadic functions (`printf`-style)
- `if`/`else`, `for`/`while`/`do-while`, `switch`, `goto`, the ternary operator
- `alloca` and local aggregates
- A subset of the C standard library, implemented on top of UVM primitives:
  [`string.h`](include/string.h), [`ctype.h`](include/ctype.h),
  [`stdlib.h`](include/stdlib.h), [`stdio.h`](include/stdio.h) (including
  `printf`/`sprintf`), [`math.h`](include/math.h), and [`pthread.h`](include/pthread.h)
- UVM platform bindings under [`include/uvm/`](include/uvm): `syscalls.h`,
  `graphics.h`, `window.h`, `music.h`, `utils.h`, `math.h`, `3dmath.h`

If uvclang hits LLVM IR it does not yet handle, it prints a diagnostic and
points you at the issue tracker. Updating to a newer clang sometimes helps (it
may emit IR the back end already supports); otherwise, please
[open an issue](https://github.com/maximecb/uvm/issues/new) so the construct can
be added. Contributions welcome.

## Usage

```
usage: uvclang <input.c|.ll> [-o out.asm] [-O0|-O1|-O2|-O3]
       [-D<macro>] [-I<dir>] [-U<macro>] [-Wall|-W<warn>] [-std=<std>]
       [-f<feature>] [--emit-ir] [--stats]
```

Compile a C program to UVM assembly:
```sh
cargo run -- examples/snake.c -o out.asm
```

If no `-o` is given, the assembly is written to stdout. The input can also be a
`.ll` file, in which case clang is skipped and the LLVM IR is compiled directly
(the back-end-only path).

Notable flags:
- `-O0` … `-O3` — optimization level, forwarded to clang (default `-O2`).
  `-O0` keeps `alloca`/load/store-heavy IR; `-O1`/`-O2` produce SSA-register-heavy
  IR, so the two exercise different back-end paths.
- `-D`, `-I`, `-U`, `-Wall`, `-std=…`, `-f…` — forwarded to clang, so they behave
  as you'd expect from a normal clang invocation.
- `--emit-ir` (alias `--emit-llvm`) — dump the front-end LLVM IR and stop, handy
  for debugging or extending back-end coverage.
- `--stats` — print module statistics (functions, basic blocks, instructions).

### Compile and run in one step

The [`build_and_run.sh`](build_and_run.sh) script compiles a source file with
uvclang and immediately runs the result in UVM. It forwards all arguments to
uvclang, and works from any directory:
```sh
./build_and_run.sh examples/snake.c
```

### Environment variables

- `CLANG` / `CLANGXX` — path to the clang / clang++ binary to use as the front end.
- `UVCLANG_INCLUDE` — override the shipped [`include/`](include) directory that
  provides the UVM headers and standard-library shims.

## UVM bindings and examples

To use the UVM APIs, include the headers under [`include/uvm/`](include/uvm),
for example `#include <uvm/syscalls.h>` or `#include <uvm/graphics.h>`. There
are whole example programs under the [`examples`](examples) directory — a
subtractive synth, an LZ77 compressor, a raycaster, Game of Life, a chess
engine, and more — that show what uvclang can compile and how to drive the UVM
platform APIs. Contributions of new examples are welcome.

## The test suite

uvclang is tested at several levels:

- `cargo test` — Rust unit tests for the back end (codegen, layout).
- [`./run_tests.sh`](run_tests.sh) — **differential** end-to-end tests. Each
  `tests/*.c` is compiled to UVM asm and run in UVM, then compiled and run
  natively with `cc`; the exit code and stdout must match. Every test is run at
  `-O0`, `-O1`, and `-O2`.
- [`./run_uvm_tests.sh`](run_uvm_tests.sh) — self-checking tests for the
  `<uvm/...>` platform headers, which have no native-libc equivalent. Each
  `tests/uvm_*.c` asserts its own results and must exit 0.
- [`./build_examples.sh`](build_examples.sh) — compiles every
  [`examples/*.c`](examples) at `-O0`/`-O1`/`-O2` and runs UVM in `--parse-only`
  mode over the output, a smoke test that the curated example programs still
  compile and parse.

The goal is to grow coverage over time and increase the range of programs
uvclang can compile and run on UVM.
