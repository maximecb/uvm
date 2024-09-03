#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <assert.h>
#include <uvm/syscalls.h>

typedef struct
{
    u16 kind;
    u16 window_id;
    u16 key;
    u16 button;
    i32 x;
    i32 y;
} Event;

// Stack allocation of structs not yet supported
Event __event__;

// Simple event loop that tries to update rendering at a fixed rate
// until the user closes the window or presses the escape key
void anim_event_loop(u64 max_fps, void* update_fn)
{
    assert(max_fps > 0);

    u64 frame_time = 1000 / max_fps;

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

        u64 start_time = time_current_ms();

        // Call the update function
        asm (update_fn) -> void { call_fp 1; };

        u64 end_time = time_current_ms();
        u64 update_time = end_time - start_time;

        if (update_time < frame_time)
        {
            thread_sleep(frame_time - update_time);
        }
    }
}

#endif
