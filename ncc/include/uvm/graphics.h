#ifndef __UVM_GRAPHICS__
#define __UVM_GRAPHICS__

#include <assert.h>
#include <uvm/syscalls.h>

#define COLOR_BLACK     0xFF_00_00_00
#define COLOR_WHITE     0xFF_FF_FF_FF
#define COLOR_GREY      0xFF_80_80_80
#define COLOR_RED       0xFF_FF_00_00
#define COLOR_GREEN     0xFF_00_FF_00
#define COLOR_BLUE      0xFF_00_00_FF
#define COLOR_ORANGE    0xFF_FF_A5_00
#define COLOR_YELLOW    0xFF_FF_FF_00
#define COLOR_MAGENTA   0xFF_FF_00_FF
#define COLOR_PURPLE    0xFF_D6_00_FF
#define COLOR_TURQUOISE 0xFF_40_E0_D0

// Convert RGB values in the range [0, 255] to a u32 encoding
#define rgb32(r, g, b) ((u32)0xFF_00_00_00 | ((u32)r << 16) | ((u32)g << 8) | (u32)b)
#define rgba32(r, g, b, a) (((u32)a << 24) | ((u32)r << 16) | ((u32)g << 8) | (u32)b)

// Fill a rectangle area with a given color
void fill_rect(
    u32* fb,
    u32 fb_width,
    u32 fb_height,
    u32 xmin,
    u32 ymin,
    u32 width,
    u32 height,
    u32 color
)
{
    if (xmin >= fb_width || ymin >= fb_height)
        return;

    if (xmin + width > fb_width)
        width = fb_width - xmin;

    if (ymin + height > fb_height)
        height = fb_height - ymin;

    for (u32 j = 0; j < height; ++j)
    {
        memset32(
            fb + fb_width * (ymin + j) + xmin,
            color,
            width
        );
    }
}

// Draw a line using Bresenham's algorithm
void draw_line(
    u32* fb,
    u32 fb_width,
    u32 fb_height,
    u32 x0,
    u32 y0,
    u32 x1,
    u32 y1,
    u32 color
)
{
    assert(x0 < fb_width && y0 < fb_height);
    assert(x1 < fb_width && y1 < fb_height);

    int dx;
    int sx;
    if (x0 < x1)
    {
        dx = x1 - x0;
        sx = 1;
    }
    else
    {
        dx = x0 - x1;
        sx = -1;
    }

    int dy;
    int sy;
    if (y0 < y1)
    {
        dy = y0 - y1;
        sy = 1;
    }
    else
    {
        dy = y1 - y0;
        sy = -1;
    }
    assert(dy <= 0);

    int err = dx + dy;

    for (;;)
    {
        // Plot one pixel
        u32* pix_ptr = fb + (y0 * fb_width) + x0;
        *pix_ptr = color;

        if(x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 >= dy)
        {
            if (x0 == x1)
                break;

            err = err + dy;
            x0 = x0 + sx;
        }

        if (e2 <= dx)
        {
            if (y0 == y1)
                break;

            err = err + dx;
            y0 = y0 + sy;
        }
    }
}

#endif
