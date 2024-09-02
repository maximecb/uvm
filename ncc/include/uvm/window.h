#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <assert.h>
#include <uvm/syscalls.h>

typedef struct
{
    u16 kind;
    u16 window_id;
    u16 key;
    u16 btn;
    i32 x;
    i32 y;
} Event;

// Simple event loop that tries to update rendering at a fixed rate
// until the user closes the window or presses the escape key
void anim_event_loop(u64 max_fps, void* update_fn)
{
    assert(max_fps > 0);

    u64 frame_time = 1000 / max_fps;

    Event event;

    for (;;)
    {
        while (window_poll_event(&event))
        {
            if (event.kind == EVENT_QUIT)
            {
                return;
            }

            if (event.kind == EVENT_KEYDOWN && event.key == KEY_ESCAPE)
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
