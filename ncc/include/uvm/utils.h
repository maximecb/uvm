#ifndef __UTILS_H__
#define __UTILS_H__

#include <assert.h>

// Set a hidden global variable to enable the UVM event loop
// This will cause your program to continue running after
// your main() function returns.
void enable_event_loop()
{
    asm () -> void
    {
        push __EVENT_LOOP_ENABLED__;
        push 1;
        store_u8;
    };
}

// Schedule a new update at a fixed rate
// This takes into account the time taken by the current update
void fixed_rate_update(u64 start_time, u64 rate_ms, void* callback)
{
    assert(rate_ms > 0);

    u64 cur_time_ms = asm () -> u64 { syscall time_current_ms; };
    u64 time_taken = cur_time_ms - start_time;

    // Compute when to do the next update
    u64 next_update = (time_taken > rate_ms)? (u64)0:(rate_ms - time_taken);

    // Schedule the next update
    asm (next_update, callback) -> void { syscall time_delay_cb; };
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
