#ifndef __UVM_GRAPHICS__
#define __UVM_GRAPHICS__

// <uvm/graphics.h> for the uvclang/UVM target. Integer-only 2D raster helpers
// (rectangle fill and Bresenham line drawing) over a 32-bit BGRA framebuffer,
// written as standard, strictly-typed clang C. memset32 is the UVM syscall from
// <uvm/syscalls.h>.
//
// Like the other <uvm/...> headers, this is UVM-build only and has no native
// equivalent, so it is validated by compiling through uvclang and self-checking
// runs in UVM rather than by the native differential harness.

#include <assert.h>
#include <stdint.h>
#include <uvm/syscalls.h>   // memset32
#include <uvm/utils.h>

#define COLOR_BLACK     0xFF000000
#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_GREY      0xFF808080
#define COLOR_RED       0xFFFF0000
#define COLOR_GREEN     0xFF00FF00
#define COLOR_BLUE      0xFF0000FF
#define COLOR_ORANGE    0xFFFFA500
#define COLOR_YELLOW    0xFFFFFF00
#define COLOR_MAGENTA   0xFFFF00FF
#define COLOR_PURPLE    0xFFD600FF
#define COLOR_TURQUOISE 0xFF40E0D0

// Convert RGB/RGBA values in the range [0, 255] to a u32 (BGRA) encoding.
#define rgb32(r, g, b) \
    ((uint32_t)0xFF000000 | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define rgba32(r, g, b, a) \
    (((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

// Fill a rectangular area with a given color.
void fill_rect(
    uint32_t* fb,
    uint32_t fb_width,
    uint32_t fb_height,
    uint32_t xmin,
    uint32_t ymin,
    uint32_t width,
    uint32_t height,
    uint32_t color
)
{
    if (xmin >= fb_width || ymin >= fb_height)
        return;

    if (xmin + width > fb_width)
        width = fb_width - xmin;

    if (ymin + height > fb_height)
        height = fb_height - ymin;

    for (uint32_t j = 0; j < height; ++j)
    {
        memset32(
            fb + fb_width * (ymin + j) + xmin,
            color,
            width
        );
    }
}

// Draw a line using Bresenham's algorithm.
// This function will panic if coordinates are outside of the viewport.
void draw_line(
    uint32_t* fb,
    uint32_t fb_width,
    uint32_t fb_height,
    uint32_t x0,
    uint32_t y0,
    uint32_t x1,
    uint32_t y1,
    uint32_t color
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
        uint32_t* pix_ptr = fb + (y0 * fb_width) + x0;
        *pix_ptr = color;

        if (x0 == x1 && y0 == y1)
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

// Draw a line using Bresenham's algorithm, but also clip the input coordinates
// so they are inside the viewport first.
void draw_line_clipped(
    uint32_t* fb,
    uint32_t fb_width,
    uint32_t fb_height,
    int32_t x0,
    int32_t y0,
    int32_t x1,
    int32_t y1,
    uint32_t color
)
{
    // Swap the coordinates so x0 <= x1
    if (x0 > x1)
    {
        int32_t tmp = x0;
        x0 = x1;
        x1 = tmp;

        tmp = y0;
        y0 = y1;
        y1 = tmp;
    }

    // If the line is out of frame, reject it
    if (x1 < 0 || x0 >= (int32_t)fb_width)
    {
        return;
    }

    // If part of the line is to the left of the frame
    if (x0 < 0)
    {
        int32_t ex = -x0;
        y0 = y0 + (y1 - y0) * ex / (x1 - x0);
        x0 = 0;
    }

    // If part of the line is to the right of the frame
    if (x1 >= (int32_t)fb_width)
    {
        // ex is a negative quantity
        int32_t ex = (fb_width - 1) - x1;
        y1 = y1 + (y1 - y0) * ex / (x1 - x0);
        x1 = x1 + ex;
    }

    // Swap the coordinates so y0 <= y1
    if (y0 > y1)
    {
        int32_t tmp = x0;
        x0 = x1;
        x1 = tmp;

        tmp = y0;
        y0 = y1;
        y1 = tmp;
    }

    // If the line is out of frame, reject it
    if (y1 < 0 || y0 >= (int32_t)fb_height)
    {
        return;
    }

    // If part of the line is above the frame
    if (y0 < 0)
    {
        int32_t ey = -y0;
        x0 = x0 + (x1 - x0) * ey / (y1 - y0);
        y0 = 0;
    }

    // If part of the line is below the frame
    if (y1 >= (int32_t)fb_height)
    {
        // ey is a negative quantity
        int32_t ey = (fb_height - 1) - y1;
        x1 = x1 + (x1 - x0) * ey / (y1 - y0);
        y1 = y1 + ey;
    }

    draw_line(
        fb,
        fb_width,
        fb_height,
        x0,
        y0,
        x1,
        y1,
        color
    );
}

#endif // __UVM_GRAPHICS__
