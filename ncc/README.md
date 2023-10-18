# NCC - Not a C Compiler

Ceci n'est pas un compilateur C.

The name NCC stands for "Not a C Compiler", because the compiler is a work in progress
and not fully compliant. At this stage, it should be considered a toy compiler and you
should expect to run into bugs and missing features. That being said, it is still
a useful tool for the development of UVM, and the goal is to gradually increase the
level of C compliance of NCC over time.

Supported features:
- Global variables
- Arrays, N-dimensional arrays
  - Array initializer syntax (global variables only)
- Structs
- Integer arithmetic and bitwise operations
- Signed and unsigned integers, `stdint.h`
- The `float` type and floating-point arithmetic
- Character strings and the `char` type
- The `bool` type
- Pointers and pointer arithmetic, bitwise operations
- The ternary operator `a? b:c`
- `sizeof()` operator
- Functions and function calls
- If/else statements
- For loops, while loops, do-while loops
- Typedefs
- C preprocessor
  - `#include`
  - `#define` constants
  - `#define(a,b,c)` macros
  - `#undef`
  - `#ifdef` / `#ifndef`
  - `__COUNTER__`, `__LINE__`, `__FILE__`
- Headers for UVM bindings
  - `#include <uvm/syscalls.h>`
  - `#include <uvm/utils.h>`
  - `#include <uvm/graphics.h>`

Not yet implemented (TODO):
- Unions
- Enums
- The `const` qualifier
- The `double` type
- The `switch` statement
- Local struct variables (can only be globals or malloc'ed)
- Initializer syntax for local array variables
- Pointers to local variables
- Struct initializer syntax

Caveats and quirks:
- You may need to manually insert type casts in places where they aren't required by the C standard.
- At the moment `malloc()` / `free()` don't use a free list, they just leak memory
  - If you would like to help with this, the implementation is in [`include/stdlib.h`](include/stdlib.h)
- The error messages are currently fairly weak
  - Type errors have no line numbers
  - Errors inside macro expansions have incorrect line numbers
- The [`printf()` function](include/stdio.h) is only partially supported
  - Contributions and improvements welcome!

## Usage

Compiling a C program:
```sh
cargo run <your_c_file.c>
```

By default, the compiled output is written to `out.asm`.

Running tests:
```sh
cargo test
```

To use the UVM bindings, you should include the
[`uvm/syscalls.h`](include/uvm/syscalls.h) header.
There are example programs under the [`examples`](examples) directory
that show the kinds of things NCC is able to compile and how to use
the UVM APIs. You can also consult the source code for the UVM
headers in the [`include`](include) directory to see which
standard library functions are currently available.
Contributions welcome.

## The Test Suite

There is a set of C source files under the [`tests`](tests) directory.
These are automatically compiled by NCC and executed by UVM when running `cargo test`.
The tests use the `assert()` macro to check that the behavior of the compiler is
as expected. The goal is to increase test coverage over time and gradually
increase the level of C compliance of NCC.
