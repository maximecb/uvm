#ifndef __UTILS_H__
#define __UTILS_H__

// <uvm/utils.h> for the uvclang/UVM target. Ported from ncc/include/uvm/utils.h.
// The benchmark() macro times a statement/expression using the millisecond clock
// syscall and prints the elapsed time. Rewritten to call the <uvm/syscalls.h>
// shims (__uvm_time_current_ms / __uvm_print_*) instead of ncc's inline
// `asm (...) { syscall ... }` extension.

#include <stdint.h>
#include <uvm/syscalls.h>   // __uvm_time_current_ms / __uvm_print_i64 / __uvm_print_str / __uvm_print_endl

// Benchmark an expression or statement, printing "<elapsed> ms".
#ifndef benchmark
#define benchmark(expr)                                       \
    {                                                         \
        uint64_t __bench_t0 = __uvm_time_current_ms();        \
        { expr; }                                             \
        uint64_t __bench_t1 = __uvm_time_current_ms();        \
        __uvm_print_i64((int64_t)(__bench_t1 - __bench_t0));  \
        __uvm_print_str(" ms");                               \
        __uvm_print_endl();                                   \
    }
#endif

#endif // __UTILS_H__
