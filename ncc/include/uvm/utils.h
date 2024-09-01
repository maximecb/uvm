#ifndef __UTILS_H__
#define __UTILS_H__

#include <assert.h>

// Macro to benchmark an expression or statement
#ifndef benchmark
#define benchmark(expr) \
{\
    u64 t0 = asm () -> u64 { syscall time_current_ms; };\
    { expr; }\
    u64 t1 = asm () -> u64 { syscall time_current_ms; };\
    u64 dt = t1 - t0;\
    asm (dt) -> void { syscall print_i64; };\
    asm (" ms") -> void { syscall print_str; };\
    asm () -> void { syscall print_endl; };\
}
#endif

#endif
