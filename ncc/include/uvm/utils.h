#ifndef __UTILS_H__
#define __UTILS_H__

#include <assert.h>

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

// Schedule a new update at a fixed rate
// This takes into account the time taken by the current update
void fixed_rate_update(u64 start_time, u64 update_rate, void* callback)
{
    assert(update_rate > 0);
    assert(update_rate <= 100);

    u64 time_taken = time_current_ms() - start_time;
    u64 update_delay = 1000 / update_rate;

    // Compute when to do the next update
    u64 next_update = (time_taken > update_delay)? (u64)0:(update_delay - time_taken);

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
