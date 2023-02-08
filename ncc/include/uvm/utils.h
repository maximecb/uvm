#ifndef __UTILS_H__
#define __UTILS_H__

void enable_event_loop()
{
    asm () -> void
    {
        push __EVENT_LOOP_ENABLED__;
        push 1;
        store_u8;
    };
}

#endif
