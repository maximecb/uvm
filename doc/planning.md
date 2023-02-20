# Planning and Evolution

## Iterate with Interpreter and APIs

The goal is for UVM to reach a point where we're confident enough with the APIs
we provide to freeze them, which doesn't mean that new APIs can't be added, but
means that the APIs people rely on will not change. However, we know that we can't
immediately come up with a perfect design from day one, so there will have to be
some amount of iteration and experimentation.
At the moment, UVM is at the prototype stage and there is no JIT compiler. We're
developing using the interpreter
because it's easier to refactor, which allows us to quickly make design changes.

## C Compiler Development

This repository includes [ncc](/ncc/README.md), which is a toy C compiler that targets UVM.
This compiler is currently incomplete, but still able to compile useful code.
Eventually, we're probably going to want to have an LLVM backend targeting UVM,
but it seems useful to have a small compiler that people can easily wrap their
head around so that they can understand how they could build their own.
The plan is to put some amount of development effort into ncc to smooth out
the rough edges, improve error messages and get it to the point where it
supports most C features.

## Binary Image Format

Currently, the only way to run code in UVM is to write said code into a
`.asm` file. Eventually, the goal is to have a binary image format that
can include code, data and also metadata about the software. This binary
format will be more compact and also faster to load. The reason we're not
developing this binary format right away is that we still need time to
iterate on the design of UVM, and text formats are easier to change. For
instance, if we rename an instruction or change the parameters of a system
call, it's easy to repair `.asm` files by hand with a text editor, but the
same thing can't be said for binary files.

## JIT Compiler

UVM has been [designed](/doc/design.md) with JIT compilation in mind, that
is, we've made multiple design choices that we think will make it easier
to generate efficient machine code from our bytecode instructions. We
believe it should be possible to get good performance with a fairly
simple JIT compiler. A speedup of 20x or more over the interpreter
should be expected, and hopefully near-native performance.

We don't want to stat working on the JIT compiler very early in the
prototype stage, because it's easier to quickly iterate over the design
while working with an interpreter, but experimentation with
JIT compilation needs to happen before we stabilize the current design.
That being said, one advantage of working with just an interpreter is
that it will motivate us to optimize code to perform better with the
performance constraints of the interpreter.

## Stabilizing Near 1.0

The ultimate goal is to stabilize and freeze the existing opcodes and
system calls as UVM nears version 1.0. This will take some time as we
need to experiment and gain confidence in the current design. The
general expectation should be that there are going to be breaking
changes in the short term, but less and less so as the design
evolves and stabilizes. Once version 1.0 is reached, the existing
features will be essentially frozen and backward compatibility
will be guaranteed.

## SIMD Support

UVM will try to provide useful mechanisms to take advantage of parallelism
on modern machines. The challenge here is how to do this in a way that
is simple, portable and predictable.
UVM isn't going to directly provide low-level SIMD instructions that do
things such as multiply 4 or 8 floating-point at once, because those
instructions are very platform-specific. Instead, we'd like to provide
some higher primitives.

Currently, UVM provides system calls such as `memset` and `memcpy`.
It might not be immediately apparent, but those operations use SIMD
instructions under the hood. Even though UVM doesn't yet have a JIT,
you can use `memcpy` to copy bytes at tens of gigabytes per second,
and you can use `memcpy` to write graphics routines and copy sprites
into a frame buffer while benefiting your CPU's SIMD capabilities.

Eventually, UVM may provide operations such as a blit primitive
with alpha blending, vector and matrix operations, or a more
general [einsum](https://rockt.github.io/2018/04/30/einsum) primitive.
Another possibility would be to implement a mechanism to run UVM
bytecode in an [SIMT](https://en.wikipedia.org/wiki/Single_instruction,_multiple_threads)
way which could be parallelized on a GPU. Parallelism is an area
of design that still needs more discussion and exploration.

## Future Extensions

The goal is for UVM to provide a stable instruction set and a stable set
of APIs. That doesn't mean, however, that we won't add new APIs to UVM.
We may want to add an API to interface with MIDI devices, for example.
An easy way to do that is to design a new set of system calls without
changing the existing system calls.

When it comes to adding new APIs and extensions to UVM, we want to
do it slowly and carefully. It's important that UVM remains relatively
small and maintainable. We don't want to add dozens of dependencies
that could break. We don't want to make UVM hard to build. We also
don't want to add half-baked APIs that people will rely on, and will
turn out to be a liability because of poor design choices.
