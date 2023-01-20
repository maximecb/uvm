# UVM

**NOTE: this project is very much a work in progress and not ready for prime time at this point. You're likely to run
into bugs and missing features, but I am looking for collaborators who share the vision.**

A minimalistic virtual machine designed to run self-contained applications. UVM is intended as a platform to distribute
programs that will not break and to combat code rot. It also aims to be conceptually simple, easy to understand, fun to work
with and approachable to newcomers.

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

## Build instructions

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

## Motivation

We live in a world where software is increasingly complex, and increasingly fragile as a result. It's very
easy to end up in a situation where software that was working just fine a few months ago can no longer
compile and run due to broken dependencies. One of the main goals of UVM is to combat the phenomenon
known as "code rot" or "bit rot". UVM will provides simple, minimalistic and robust APIs. Most importantly,
after a certain experimentation period, existing APIs will be essentially frozen and guaranteed to remain
stable. That will ensure that software built to run on UVM will have a much better chance of working,
hopefully years or even decades into the future.

The idea of a virtual machine is obviously not new. We're all familiar with the JVM. You can also think
of the web platform as a VM. The key differences are that UVM will be much smaller, and that its APIs will
be much simpler. UVM isn't going to provide complex, high-level APIs like a GUI toolkit, because different
implementations of such an API would be almost guaranteed to behave differently, in the same way that
modern web browsers can't manage to produce an identical rendering of the same webpage.

There seems to be a growing interest in retrocomputing, and that interest likely stems, in large part,
because the complexity of modern computer systems and their software environment is extremely high, and
there is constant unnecessary churn becomes exhausting. UVM aims to provide a platform that is simple,
stable over time, and easy to target and develop for. I hope that this platform will bring back the joy
of programming for many.

If you'd like to read more about the philosophy behind UVM's design, here are some relevant blog posts:
- [Code that Doesn't Rot](https://pointersgonewild.com/2022/02/11/code-that-doesnt-rot/)
- [Typed vs Untyped Virtual Machines](https://pointersgonewild.com/2022/06/08/typed-vs-untyped-virtual-machines/)
- [The Need for Stable Foundations in Software Development](https://pointersgonewild.com/2020/09/22/the-need-for-stable-foundations-in-software-development/)
- [Minimalism in Programming](https://pointersgonewild.com/2018/02/18/minimalism-in-programming/)

## Design Goals

UVM is designed with the following goals in mind.

**Minimalism**: to minimize the risk of breakage, keep the VM as small as possible. UVM tries to implement as few instructions as possible,
and uses an untyped design. UVM does not implement a garbage collector or type checking. The APIs it provides are low-level and have as
little surface area as possible. Each bytecode instruction should have as little dynamic behavior and as few special cases as possible.

**Portability**: the minimalistic design of UVM makes portability easier to achieve since it is conceptually simpler than other VMs and has
fewer dependencies.

**Stability**: UVM aims to provide a foundation that remains as stable as possible over time. Ideally, existing instructions and APIs should
never change, to avoid ever breaking software that relies on UVM. The VM design is evolved slowly and carefully, and we are conservative in
choosing to implement new features.

**Self-contained apps**: programs that runs on UVM are self-contained in a single-file application image format. All application images must be
statically linked. This makes software easier to distribute and less likely to break.

**Easy to target**: UVM aims to be well-documented and easy to target. Its minimalistic philosophy and simple APIs make this easier to achieve.

**Performance**: although untyped, UVM is designed so that its bytecode can be easily JIT-compiled using dynamic binary translation techniques
so that it can provide a good performance level that will be adequate for most applications.

**Pragmatism**: because of its minimalistic design, UVM will not be suitable for every possible use case. Its main target is software that end users
run, as well as some network services. Those who want to squeeze absolute peak performance out of an application or rely on the latest libraries
may be disappointed. However, we believe that trying to implement every feature and satisfy every possible use case would only make the design
more fragile and detract from our design goals.

## Design Choices

### UVM provides a frame buffer, but it doesn't provide GUI APIs

The APIs provided by UVM are intentionally kept simple and low-level. To produce graphics, UVM provides an API to draw a pixel buffer one frame at a time into a window. It doesn't provide a Graphical User Interface (GUI) toolkit as part of the VM itself. The reason for that is simple, it's because GUIs are complex and any GUI API that you design will inevitably have a large API surface. Another problem is that a high-level GUI will inevitably keep growing as users request more and more features that other GUI frameworks offer. If we want UVM to be small, portable, and to behave the same everywhere, then including complex APIs in the VM itself will necessarily become a liability.

UVM provides a frame buffer API. The API is dead simple, you give it an array of pixels, and it shows the frame in a window. This puts the burden on the software to implement its own GUI. This might seem awful to some, but it also means you have complete freedom and control over what your GUI looks like, and you can be guaranteed that it will look the same on any machine you try it on. Furthermore, once one person implements a GUI library for UVM, this code can be reused.

You may feel concerned about the performance of rendering GUIs in software, but there are a few simple things that UVM can do to make things faster. As it stands, UVM provides a system call to perform the `memcpy` operation. This may not seem like much, but it allows programs running on UVM to copy rows of pixels using SIMD operations running on the host machine instead of going pixel by pixel. In the future, we may provide more sophisticated blitting operations inspired by the Amiga's bit blit or using alpha blending, and/or more general APIs for SIMD computations.

### Why a 64-bit machine? Why not 32-bit or 16-bit?

UVM is a 64-bit machine in the sense that each stack slot holds 64 bits, and it provides 64-bit integer arithmetic operations. 
This is a pragmatic hoice as most modern microprocessors on PCs, laptops, tablets, cellphones and supercomputers are 64-bit designs. There are still 32-bit microprocessors out there, but they are the minority by far. However, note that it should be very feasible to port UVM to a 32-bit machine.
Also note that UVM also provides 32-bit integer arithmetic operations, and that if the programs you want to run on UVM use less than 4GiB of memory, you can use 32-bit pointers, which are more compact than 64-bit pointers. This means you can effectively run 32-bit programs on UVM without the memory overhead of 64-bit pointers. You could even use 16-bit pointers if you wanted to.

### Why a stack machine and not a register machine?

Stack machines have the advantage that they're conceptually simple, easy to generate code for and easy to target. Some people will argue that register machines offer better performance, but that is mostly only true in the context of interpreters. The goal is for UVM to eventually include a JIT compiler, which will make the bytecode run at native or near-native speeds.

### Why variable-length instructions instead of fixed-length instructions?

UVM uses variable-length instructions, where the shortest and most common instructions can be as short as one byte. This makes UVM bytecode more
compact, and it also simplifies decoding, as well as making UVM easier to target. On ARMv8, for instance, all instructions are 32-bytes long, but
moving a 64-bit pointer into a register can take several instructions, because the size of immediates that can be encoded is limited. With UVM,
64-bit constants can be loaded with a single instruction that is 9 bytes long, but pushing the value 0 to the stack uses an instruction that is just
one byte long. There is an argument to be made that an interpreter with fixed-length instructions may potentially be faster, but the ultimate goal
is for UVM to have a simple JIT compiler, which makes this point moot.

### Why little-endian?

This again comes down to pragmatism. The most widely used ISAs today (x86, ARM and RISC-V) are all little-endian architectures. Going with a big-endian design would mean adding extra overhead on most of these systems.

### Why not use x86 instructions or make this more similar to a virtual PC?

When designing a virtual platform to run software on, it's tempting to base its design on an existing system. For example, a virtual x86 PC running some barebones Linux kernel. The problem here is that this design can easily get very complex. You have to ask how much of this system you want to simulate. Are you going to handle I/O by simulating PCI devices and SATA hard drives? The more complex the design gets, the harder it is to port. The higher the risk that something will break, or that different implementations will have subtle differences in behavior. The goal with UVM is very much to design a VM that is conceptually simple, and has as few unspecified behaviors as possible.

There's a downside in UVM having its own unique architecture, which is that it can't necessarily benefit from the wealth of tools that target x86 or other existing platforms. However, there's also a flipside to this. If UVM were a virtual x86 architecture, for example, then people would necessarily come to expect that UVM will support all x86 instructions. If UVM were a virtual Linux system, then people would complain when UVM doesn't support features they've come to expect from other Linux systems. UVM has its own design and its own identity, which avoids setting the expectation that UVM has to support whatever other platform X supports.

### Why a Harvard architecture?

UVM uses a Harvard architecture, meaning it has separate memory spaces for code and data. The downside here is that you need special instructions to write to the code memory space. You can still write a JIT compiler that runs inside UVM, or self-modifying code, but you can't just store a value into the code space, you have to do it through a special UVM system call.

The reason for this is that this makes the implementation of an efficient JIT compiler easier. If UVM only had a single memory space, then a JIT compiler would have to assume that every store instruction can possibly overwrite existing core. That would mean each store instruction has to perform additional run-time checks so that JIT compiled code can be invalidated if necessary. By requiring that all writes to the code space use a specific system call, UVM knows when you are (over)writing code, and it also knows that regular store instructions can't possibly overwrite code.

### Why doesn't UVM implement a garbage collector?

This comes down to two main reasons. The first is a desire to keep the VM simple and minimalistic. Many programs don't need a GC, and if you need one,
you can implement one yourself. The second reason is that garbage collection is an entire field of research. UVM doesn't want to have to pick a
certain kind of GC, because inevitably, it would be more suitable for some kinds of software than others. By implementing your own GC, you can choose
whichever algorithm suits your needs. You can also guarantee that your GC implementation isn't going to change unless you make that choice.

### Will it be possible to do code patching in UVM, or to write a JIT that runs inside UVM?

Yes. This is very much a design goal of the system. In order to enable executing dynamic languages with good performance, it's going to be possible
to implement a JIT compiler that runs inside UVM. This will be done by writing UVM bytecode instructions into the UVM code memory space, which will
then in turn be translated into machine code by UVM's own JIT compiler. Writing to the code space will be done through specialized system calls,
and it will also be possible to disable these if desired for security reasons. That is, if your application is fully statically compiled and you have
no need to generate code on the fly, you can disable this feature for enhanced security.
