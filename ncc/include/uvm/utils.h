#ifndef __UTILS_H__
#define __UTILS_H__

void enable_event_loop()
{
    // Set a global variable to enable the event loop
    asm () -> void
    {
        push __EVENT_LOOP_ENABLED__;
        push 1;
        store_u8;
    };
}

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
