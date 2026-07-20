# UVM

**NOTE: this project is very much a work in progress. You're likely to run
into bugs and missing features. I would like to find collaborators who share the vision
and want to help me make it happen.**

<p align="center">
    <img src="media/the_grid.png" height=180>&nbsp;
    <img src="media/attackers.png" height=180>&nbsp;
    <img src="media/chess.png" height=180>
</p>

A simple, minimalistic virtual machine designed to run self-contained applications. UVM is intended as a platform to distribute
programs that will not break and to combat code rot. It aims to be conceptually simple, easy to understand, easy
to target, fun to work with and approachable to newcomers. It may also be valuable as a teaching tool or as a platform
to experiment with. There is a short 4-minute [overview of UVM](https://www.youtube.com/watch?v=q9-o45B_qsA)
on YouTube if you'd like to see a quick survey.

Contents:
- [Features](#features)
- [Build Instructions](#build-instructions)
- [Codebase Organization](#codebase-organization)
- [Design and Architecture](docs/design.md)
- [Subsystems and System Calls](docs/syscalls.md)
- [Vision and Motivation](docs/vision.md)
- [Planning and Evolution](docs/planning.md)

If you think that UVM is cool, you can support my work via [GitHub Sponsors](https://github.com/sponsors/maximecb) :heart:

## Features

Current features:
- Stack-based bytecode interpreter
- Variable-length instructions for compactness
- Untyped design for simplicity
- Instruction set designed for straightforward JIT compilation to x86-64, arm64 and RV64
- Little-endian byte ordering (like x86, ARM & RISC-V)
- 32-bit and 64-bit integer ops, 32-bit floating-point support
- Separate flat, linear address spaces for code and data ([Harvard architecture](https://en.wikipedia.org/wiki/Harvard_architecture))
- Thread-based parallelism
- Built-in, easy to use [assembler](vm/src/asm.rs) with a [simple syntax](vm/examples)
- Event-driven event execution model compatible with async operations
- Easy to use frame buffer to draw RGB graphics with no boilerplate
- Easy to use audio output API with no boilerplate
- Simple TCP networking API
- It can [run Doom](https://github.com/maximecb/uvm-doom)

Planned future features:
- Capability system to safely sandbox apps without granting access to entire computer
- Ability to compile without SDL and without graphics/audio for headless server-side use
- Ability to encode metadata such as author name and app icon into app image files
- Ability to suspend running programs and save them to a new app image file

## Build Instructions

Dependencies:
- The [Rust toolchain](https://www.rust-lang.org/tools/install)
- The [SDL2 libraries](https://wiki.libsdl.org/SDL2/Installation)

### Installing Rust and SDL2 on macOS

Install the SDL2 package:
```sh
brew install sdl2
```

Add this to your `~/.zprofile`:
```sh
export LIBRARY_PATH="$LIBRARY_PATH:$(brew --prefix)/lib"
```

Install the Rust toolchain:
```sh
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

### Installing Rust and SDL2 on Debian/Ubuntu

Install the SDL2 package:
```sh
sudo apt-get install libsdl2-dev
```

Install the Rust toolchain:
```sh
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

### Installing Rust and SDL2 on Windows

Follow the Windows-specific instructions to [install the Rust toolchain](https://www.rust-lang.org/tools/install).

Get `SDL2.dll` from one of [SDL2 Releases](https://github.com/libsdl-org/SDL/releases).

Copy `SDL2.dll` (unzip) to the `vm/` folder.

### Compiling the Project

```sh
cd vm
cargo build
```

To run an [asm file](vm/examples) with UVM:
```sh
cargo run examples/fizzbuzz.asm
```

There is also a C compiler, [uvclang](uvclang/README.md), in the `uvclang` directory, along with many [example C programs](uvclang/examples) that run on UVM:
```sh
cd uvclang
./build_and_run.sh examples/snake.c
```
uvclang uses clang as a front end, so it needs a working clang/LLVM installation. See the [uvclang README](uvclang/README.md) for details.

### Running the Test Suite

Run `cargo test` from the `vm` and `uvclang` directories. uvclang also has shell-based end-to-end test harnesses; see the [uvclang README](uvclang/README.md).

## Codebase Organization

The repository is organized into a 3 different subprojects, each of which is a Rust codebase which can be compiled with `cargo`:

- `/vm` : The implementation of the UVM virtual machine itself
  - [`/vm/examples/*`](vm/examples): Example assembly programs that can be run by UVM
- `/uvclang`: A C/C++ compiler that outputs UVM assembly, using clang as a front end
  - [`/uvclang/README.md`](uvclang/README.md): documentation for the uvclang compiler
  - [`/uvclang/examples/*`](uvclang/examples): Example C source files that can be compiled by uvclang
- `/spec`: A system to document and automatically export bindings for UVM system calls and constants.
  - `/spec/syscalls.json`: Declarative list of system calls exposed by UVM.
- `/docs`: Markdown documentation for UVM
  - [`/docs/syscalls.md`](docs/syscalls.md): List of system calls and constants accessible to UVM programs

The `uvclang` compiler drives clang to lower C/C++ to LLVM IR and then compiles that IR to UVM assembly. Because clang handles parsing, it supports the
full C/C++ language; the work in progress is in the LLVM IR to UVM back end, which does not yet cover every construct clang can emit. It is already usable
to write real programs, and ships with a set of UVM bindings and standard-library headers. Contributions are welcome.

The `spec` directory contains JSON files that represent a declarative list of system calls, constants and the permission system that UVM exposes
to programs running on it. This is helpful for documentation purposes, or if you want to build a compiler that targets UVM. The directory also contains
code that automatically generates [markdown documentation](docs/syscalls.md), Rust constants and [C definitions](uvclang/include/uvm/syscalls.h) for system calls.

## Open Source License

The code for UVM, uvclang and associated tools is shared under the [Apache-2.0 license](https://github.com/maximecb/uvm/blob/main/LICENSE).

The examples under the `vm/examples` and `uvclang/examples` directories are shared under the [Creative Commons CC0](https://creativecommons.org/publicdomain/zero/1.0/) license.

## Contributing

There is a lot of work to be done to get this project going and contributions are welcome.

A good first step is to look at open issues and read the available documentation. Another easy way to contribute
is to create new example programs showcasing cool things you can do with UVM, or to open issues to report bugs.
If you do report bugs, please provide as much context as possible, and the smallest reproduction you can
come up with.

You can also search the codebase for TODO or FIXME notes:
```sh
grep -IRi "todo" .
```

In general, smaller pull requests are easier to review and have a much higher chance of getting merged than large
pull requests. If you would like to add a new, complex feature or refactor the design of UVM, I recommend opening
an issue or starting a discussion about your proposed change first.

Also please keep in mind that one of the core principles of UVM is to minimize dependencies to keep the VM easy
to install and easy to port. Opening a PR that adds dependencies to multiple new packages and libraries is
unlikely to get merged. Again, if you have a valid argument in favor of doing so, please open a discussion to
share your point of view.
