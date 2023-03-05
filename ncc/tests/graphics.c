#include <uvm/syscalls.h>
#include <uvm/graphics.h>
#include <assert.h>

// Frame buffer
u32 fb[800][600];

int main()
{
    assert(rgb32(0, 0, 0) == 0xFF_00_00_00);
    assert(rgb32(255, 0, 0) == 0xFF_FF_00_00);
    assert(rgb32(0, 255, 0) == 0xFF_00_FF_00);
    assert(rgb32(0, 0, 255) == 0xFF_00_00_FF);
    assert(COLOR_RED > 0);

    memset32(fb, COLOR_GREY, 800 * 600);

    draw_line(
        (u32*)fb, 800, 600,
        0, 0,
        250, 222,
        rgb32(255, 255, 255)
    );

    return 0;
}
