#include <uvm/syscalls.h>
#include <uvm/graphics.h>
#include <uvm/utils.h>
#include <assert.h>

// Frame buffer
u32 fb[800][600];

int main()
{
    assert(rgb32(0, 0, 0) == 0xFF_00_00_00);
    assert(rgb32(255, 0, 0) == 0xFF_FF_00_00);
    assert(rgb32(0, 255, 0) == 0xFF_00_FF_00);
    assert(rgb32(0, 0, 255) == 0xFF_00_00_FF);
    assert(rgba32(0, 0, 255, 128) == 0x80_00_00_FF);

    assert(COLOR_RED > 0);

    memset32(fb, COLOR_GREY, 800 * 600);

    fill_rect(
        (u32*)fb, 800, 600,
        200, 100,
        200, 80,
        COLOR_BLUE
    );

    draw_line(
        (u32*)fb, 800, 600,
        50, 50,
        700, 522,
        rgb32(255, 255, 255)
    );

    /*
    window_create(800, 600, "Graphics Test", 0);
    window_draw_frame(0, fb);
    enable_event_loop();
    */

    return 0;
}
