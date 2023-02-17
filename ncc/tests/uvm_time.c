#include <stdlib.h>
#include <assert.h>
#include <uvm/syscalls.h>
#include <uvm/utils.h>

bool cb1 = false;
bool cb2 = false;

void callback1()
{
    if (cb2)
        exit(0);

    cb1 = true;
}

void callback2()
{
    if (cb1)
        exit(0);

    cb2 = true;
}

void main()
{
    u64 t0 = time_current_ms();
    u64 t1 = time_current_ms();
    assert(t1 >= t0);

    // This test should only terminate if both callbacks are called,
    // otherwise it will hang
    time_delay_cb(1, callback1);
    time_delay_cb(2, callback2);
    enable_event_loop();
}
