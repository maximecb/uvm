# UVM

A minimalistic virtual machine designed to run self-contained applications. UVM is intended as a platform to distribute
programs that will not break and to combat code rot. It also aims to be conceptually simple, easy to understand, fun to work
with and approachable to newcomers.

**NOTE: this project is very much WIP and not ready for production, but I am looking for contributors who share
the vision :)**

## Features

- Stack-machine interpreter
  - Eventually to include a JIT compiler
- Untyped design, manipulates 64-bit words
- Little-endian byte ordering (like x86, ARM & RISC-V)
- Variable-length instructions for compactness
- Linear address spaces for data and code
- Separate address spaces for code and data
  - For security and to facilitate JIT compilation
- Built-in, easy to use assembler
- Provides an easily accessible frame buffer to draw RGB graphics

## Build instructions

Dependencies:
- The [Rust toolchain](https://www.rust-lang.org/tools/install)
- The SDL2 libraries

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
cargo build
```

To run the compiled binary:
```
cargo run <input_file>
```

## Design Goals

UVM is designed with the following goals in mind.

**Minimalism**: to minimize the risk of breakage, keep the VM as small as possible. UVM tries to implement as few instructions as possible,
and uses an untyped design. UVM does not implement a garbage collector or type checking. The APIs it provides are low-level and have as
little surface area as possible. Each bytecode instruction should have as little dynamic behavior and as few special cases as possible.

**Portability**: the minimalistic design of UVM makes portability easier to achieve.

**Stability**: UVM aims to provide a foundation that remains as stable as possible over time. Ideally, existing instructions and APIs should
never change, to avoid ever breaking software that relies on UVM. The VM design is evolved slowly and carefully, and we are conservative in
choosing to implement new features.

**Self-contained**: programs that runs on UVM are self-contained in a single-file application image format. All application images must be
statically linked. This makes software easier to distribute and less likely to break.

**Easy to target**: UVM aims to be well-documented and easy to target. Its minimalistic philosophy and simple APIs make this easier to achieve.

**Performance**: although untyped, UVM is designed so that its bytecode can be easily JIT-compiled using dynamic binary translation techniques
so that it can provide a good performance level that will be adequate for most applications.

**Pragmatism**: because of its minimalistic design, UVM will not be suitable for every possible use case. Its main target is software that end users
run, as well as some network services. Those who want to squeeze absolute peak performance out of an application or rely on the latest libraries
may be disappointed. However, we believe that trying to implement every feature and satisfy every possible use case would only make the design
more fragile and detract from our design goals.

## Q&A

### Q: Why a 64-bit machine? Why not 32-bit or 16-bit?

This is a matter of pragmatism. Most modern microprocessors on PCs, laptops, tablets, cellphones and supercomputers are 64-bit designs. There are still 32-bit microprocessors out there, but they are the minority by far. Note that it should be very feasible to port UVM to a 32-bit machine. Also note that if the programs you want to run on UVM use less than 4GiB of memory, you can use 32-bit pointers, which are more compact than 64-bit pointers. This means you can effectively run 32-bit programs on UVM without the data size overhead of 64-bit pointers. You could even use 16-bit pointers if you wanted to.

### Q: Why a stack machine?

Stack machines have the advantage that they're conceptually simple, easy to generate code for and easy to target. Some people will argue that register machines offer better performance, but that is mostly only true in the context of interpreters. The goal is for UVM to eventually include a JIT compiler, which will make the bytecode run at native or near-native speeds.

### Q: Why little endian?

This again comes down to pragmatism. The most widely used ISAs (x86, ARM and RISC-V) are all little-endian architectures. Going with a big-endian design would mean adding extra overhead on most of these systems.

### Q: Why not use x86 instructions or make this more similar to a virtual PC?

When designing a virtual platform to run software on, it's tempting to base its design on an existing system. For example, a virtual x86 PC running some barebones Linux kernel. The problem here is that this design can easily get very complex. You have to ask how much of this system you want to simulate. Are you going to handle I/O by simulating PCI devices and SATA hard drives? The more complex the design gets, the harder it is to port. The higher the risk that something will break, or that different implementations will have subtle differences in behavior. The goal with UVM is very much to design a VM that is conceptually simple, and has as few unspecified behaviors as possible.

There's a downside in UVM having its own unique architecture, which is that it can't necessarily benefit from the wealth of tools that target x86 or other existing platforms. However, there's also a flipside to this. If UVM were a virtual x86 architecture, for example, then people would necessarily come to expect that UVM will support all x86 instructions. If UVM were a virtual Linux system, then people would complain when UVM doesn't support features they've come to expect from other Linux systems. UVM has its own design and its own identity, which avoids setting the expectation that UVM has to support whatever other platform X supports.

### Q: Why a Harvard architecture?

UVM uses a Harvard architecture, meaning it has separate memory spaces for code and data. The downside here is that you need special instructions to write to the code memory space. You can still write a JIT compiler that runs inside UVM, or self-modifying code, but you can't just store a value into the code space, you have to do it through a special UVM system call.

The reason for this is that this makes the implementation of an efficient JIT compiler easier. If UVM only had a single memory space, then a JIT compiler would have to assume that every store instruction can possibly overwrite existing core. That would mean each store instruction has to perform additional run-time checks so that JIT compiled code can be invalidated if necessary. By requiring that all writes to the code space use a specific system call, UVM knows when you are (over)writing code, and it also knows that regular store instructions can't possibly overwrite code.
