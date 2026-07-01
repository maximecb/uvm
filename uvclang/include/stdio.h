#ifndef __STDIO_H__
#define __STDIO_H__

// <stdio.h> for the uvclang/UVM freestanding target. Ported from
// ncc/include/stdio.h, but written as standard C that clang lowers to LLVM IR
// (rather than ncc's `asm (...) { syscall ... }` extension). The character and
// string output primitives are UVM syscalls, pulled in from <uvm/syscalls.h>.
//
// This header is used ONLY for the UVM build (clang with -Iuvclang/include). The
// native reference build in the test harness uses the platform's own libc, so
// these functions are checked differentially against the real thing.
//
// NOT YET PORTED: printf() and friends. printf is variadic, and clang compiles a
// variadic *callee* down to llvm.va_start / va_arg, which uvclang does not lower
// yet (see uvclang/tests/variadic.c and the callee-side va_arg item in
// uvclang/plan.md Phase 8). Once that lands, printf can be built on top of
// putchar / print_str the way ncc's does.

#include <uvm/syscalls.h>   // __uvm_print_str / __uvm_print_endl / putchar / getchar macros

#define EOF (-1)

// Write a string followed by a newline to standard output. Standard puts
// returns a non-negative value on success; we return 0 (as ncc does). The
// differential harness compares the bytes written, not this return value.
int puts(const char *str)
{
    __uvm_print_str(str);
    __uvm_print_endl();
    return 0;
}

// putchar / getchar are already provided as function-like macros by
// <uvm/syscalls.h> (mapping directly to the `putchar` / `getchar` syscalls), so
// we only define the fallbacks if those macros are somehow unavailable.
#ifndef putchar
int putchar(int ch)
{
    return __uvm_putchar(ch);
}
#endif

#ifndef getchar
int getchar(void)
{
    return __uvm_getchar();
}
#endif

#endif // __STDIO_H__
