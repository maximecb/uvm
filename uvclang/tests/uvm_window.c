// Compile-through test for <uvm/window.h>: anim_event_loop drives a live window
// (poll events, call the nullary update callback via an indirect call, sleep to
// cap the frame rate), so it cannot run headless. A volatile guard keeps the
// call in the emitted IR -- so uvclang must lower the whole loop, including the
// `call_fp 0` -- without ever entering it, and main exits 0.
#include <uvm/window.h>

static void update(void) { }

int main()
{
    volatile int go = 0;
    if (go)
        anim_event_loop(60, update);
    return 0;
}
