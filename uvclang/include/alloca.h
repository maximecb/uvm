#ifndef __ALLOCA_H__
#define __ALLOCA_H__

// <alloca.h> for the uvclang/UVM freestanding target.
//
// alloca(size) allocates `size` bytes in the calling function's stack frame;
// the memory is freed automatically when that function returns. It cannot be an
// ordinary function -- a call would free the memory on its own return -- so, as
// on every toolchain, it is the compiler builtin `__builtin_alloca`, which
// clang expands inline to an LLVM `alloca` instruction. uvclang lowers that to a
// bump of the software stack pointer and reclaims it in the function epilogue
// (see gen_dynamic_alloca in the back end).
//
// Caveats, matching a native target:
//   * Overrunning the stack is undefined behavior (there is no bounds check).
//   * alloca() inside a loop accumulates until the function returns; use a C99
//     variable-length array if you need per-iteration reclaim.

#include <stddef.h>          // size_t

#ifndef alloca
#define alloca(size) __builtin_alloca(size)
#endif

#endif // __ALLOCA_H__
