#ifndef __ASSERT_H__
#define __ASSERT_H__

// <assert.h> for the uvclang/UVM freestanding target, written as standard C:
// assert(expr) tests the predicate and, on failure, reports "file:line +
// expression" and terminates. Standard <assert.h> routes a failure through
// __assert_fail / abort(); here we print to stdout and exit non-zero, using the
// UVM syscalls in <uvm/syscalls.h>.
//
// This header is used ONLY for the UVM build (clang with -Iuvclang/include). The
// native reference build uses the platform's own <assert.h>. The two only differ
// on the *failure* path (message text, stderr-vs-stdout, exit code); the
// differential harness never exercises that path (tests assert true predicates),
// so on the passing path assert() is a no-op in both builds.
//
// Unlike the standard header, this is written once (no re-inclusion behavior for
// NDEBUG toggling between includes), matching the rest of uvclang's headers.

#include <uvm/syscalls.h>   // __uvm_print_str / __uvm_print_i64 / __uvm_print_endl / __uvm_exit

// The failure handler carries weak linkage (UVCLANG_WEAK) so this header can be
// included from any number of translation units without producing a
// duplicate-symbol error when they are linked together (LLVM keeps a single
// copy). No per-file "implementation" opt-in is needed; in a single-translation-
// unit build the attribute has no effect and uvclang ignores it.
#ifndef UVCLANG_WEAK
#define UVCLANG_WEAK __attribute__((weak))
#endif

#ifdef NDEBUG

#define assert(ignore) ((void)0)

#else

// Report a failed assertion and terminate. Kept out of line so the assert()
// macro expands to a cheap branch on the common (passing) path.
UVCLANG_WEAK void __uvclang_assert_fail(const char *expr, const char *file, int line)
{
    __uvm_print_str("assertion failed: ");
    __uvm_print_str(expr);
    __uvm_print_str(" (");
    __uvm_print_str(file);
    __uvm_print_str(":");
    __uvm_print_i64(line);
    __uvm_print_str(")");
    __uvm_print_endl();
    __uvm_exit(-1);
}

#define assert(expr) \
    ((expr) ? (void)0 : __uvclang_assert_fail(#expr, __FILE__, __LINE__))

#endif // NDEBUG

#endif // __ASSERT_H__
