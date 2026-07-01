#ifndef __WINDOW_H__
#define __WINDOW_H__

// <uvm/window.h> for the uvclang/UVM target. Ported from
// ncc/include/uvm/window.h. A simple fixed-rate animation event loop over the
// windowing syscalls. Rewritten as standard, strictly-typed clang C: ncc's
// `u16`/`i32`/`u64` become stdint types, and the indirect update call uses a
// standard function pointer instead of ncc's inline `asm (update_fn) { call_fp
// 1; }`. The update callback is nullary (`void update()` in every ncc example),
// so this lowers to a `call_fp 0`.
//
// This header drives a live window, so unlike the other <uvm/...> headers it is
// validated by compiling through uvclang (it cannot run headless).

#include <assert.h>
#include <stdint.h>
#include <uvm/syscalls.h>

typedef struct
{
    uint16_t kind;
    uint16_t window_id;
    uint16_t key;
    uint16_t button;
    int32_t x;
    int32_t y;
    char text[64];
} Event;

// Scratch event, filled in by window_poll_event.
Event __event__;

// Simple event loop that tries to update rendering at a fixed rate until the
// user closes the window or presses the escape key. `update_fn` is a pointer to
// a nullary `void update(void)` callback that renders one frame.
void anim_event_loop(uint64_t max_fps, void* update_fn)
{
    assert(max_fps > 0);

    uint64_t frame_time = 1000 / max_fps;

    for (;;)
    {
        while (window_poll_event(&__event__))
        {
            if (__event__.kind == EVENT_QUIT)
            {
                return;
            }

            if (__event__.kind == EVENT_KEYDOWN && __event__.key == KEY_ESCAPE)
            {
                return;
            }
        }

        uint64_t start_time = time_current_ms();

        // Call the update function (nullary callback).
        ((void (*)(void))update_fn)();

        uint64_t end_time = time_current_ms();
        uint64_t update_time = end_time - start_time;

        if (update_time < frame_time)
        {
            thread_sleep(frame_time - update_time);
        }
    }
}

#endif // __WINDOW_H__
