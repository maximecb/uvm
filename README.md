# UVM

**NOTE: this project is very much a work in progress and not ready for prime time at this point. You're likely to run
into bugs and missing features, but I am looking for collaborators who share the vision.**

A minimalistic virtual machine designed to run self-contained applications. UVM is intended as a platform to distribute
programs that will not break and to combat code rot. It also aims to be conceptually simple, easy to understand, fun to work
with and approachable to newcomers.

Contents:
- [Features](#features)
- [Build Instructions](#build-instructions)
- [Codebase Organization](#codebase-organization)
- [Vision and Motivation](doc/vision.md)
- [Design and Architecture](docs/design.md)
- [Subsystems and System Calls](doc/syscalls.md)
- [Planning and Evolution](docs/planning.md)

## Features

Current features:
- Stack-based bytecode interpreter
- Variable-length instructions for compactness
- Untyped design for simplicity
- Little-endian byte ordering (like x86, ARM & RISC-V)
- Separate flat, linear address spaces for code and data
- Built-in, easy to use assembler with a simple syntax
- Easily accessible frame buffer to draw RGB graphics
- Event-driven event execution model

Planned future features:
- Easy to use audio output API
- Async file and network I/O with callbacks
  - Synchronous I/O possible as well
- Fast JIT compiler based on dynamic binary translation and basic block versioning
  - Expected performance ~80% of native speed (maybe more?)
  - Near-instant warmup
- Permission system to safely sandbox apps without granting access to entire computer
- Ability to compile without SDL and without graphics/audio for headless server-side use
- Ability to encode metadata such as author name and app icon into app image files
- Ability to suspend running programs and save them to a new app image file

## Build Instructions

Dependencies:
- The [Rust toolchain](https://www.rust-lang.org/tools/install)
- The [SDL2 libraries](https://wiki.libsdl.org/SDL2/Installation)

To install SDL2 on MacOS:
```
brew install sdl2
```

On MacOS, add this to `~/.zprofile`:
```
export LIBRARY_PATH="$LIBRARY_PATH:$(brew --prefix)/lib"
```

To install SDL2 on Debian/Ubuntu
```
sudo apt-get install libsdl2-dev
```

Install the Rust toolchain:
```
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

Compile the project:
```
cd vm
cargo build
```

To run the compiled UVM binary:
```
cargo run <input_file>
```

## Codebase Organization

The repository is organized into a 3 different subprojects:

- `/vm` : The implementation of the UVM virtual machine itself
  - `/vm/examples/*`: Example assembly programs that can be run by UVM
- `/ncc`: An implementation of a C compiler that outputs UVM assembly
  - `/ncc/examples/*`: Example C source files that can be compiled by `ncc`
- `/api`: A system to document and automatically export bindings for UVM system calls and constants.

The `ncc` compiler is, at the time of this writing, incomplete in that it lacks some C features and the error messages need improvement. This compiler
was implemented to serve as an example of how to write a compiler that targets UVM, and to write some library code to be used by other programs. Over
time, the `ncc` compiler will be improved. Despite its limitations, it is still usable to write small programs. Contributions to it are welcome.

The `api` directory contains JSON files that represent a declarative listing of system calls, constants and the permission system that UVM exposes
to programs running on it. This is helpful for documentation purposes, or if you want to build a compiler that targets UVM. The directory also contains
code that automatically generate Rust constants and C function definitions for system calls.
