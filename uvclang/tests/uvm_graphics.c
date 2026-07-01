// Self-checking test for <uvm/graphics.h>: fill_rect (memset32-backed),
// draw_line (Bresenham) and draw_line_clipped, drawing into a malloc'd
// framebuffer and asserting individual pixels. Exits 0 iff every check passes.
#include <stdlib.h>
#include <uvm/graphics.h>

int main()
{
    volatile int s = 16;
    uint32_t w = (uint32_t)s;
    uint32_t h = (uint32_t)s;
    uint32_t *fb = (uint32_t *)malloc((size_t)w * h * sizeof(uint32_t));

    memset32(fb, 0, (uint64_t)w * h);                     // clear the framebuffer

    // fill_rect fills x in [2, 6) and y in [2, 6).
    fill_rect(fb, w, h, 2, 2, 4, 4, 0xAABBCCDD);
    assert(fb[3 * w + 3] == 0xAABBCCDD);
    assert(fb[5 * w + 5] == 0xAABBCCDD);
    assert(fb[6 * w + 6] == 0);                           // just outside the rect

    // Horizontal run at y = 8, x in [0, 5].
    draw_line(fb, w, h, 0, 8, 5, 8, 0x11223344);
    assert(fb[8 * w + 0] == 0x11223344);
    assert(fb[8 * w + 5] == 0x11223344);
    assert(fb[8 * w + 6] == 0);                           // line stops at x = 5

    // Clipped line: the left endpoint (x = -4) is clipped to the viewport.
    draw_line_clipped(fb, w, h, -4, 10, 4, 10, 0x55667788);
    assert(fb[10 * w + 0] == 0x55667788);
    assert(fb[10 * w + 4] == 0x55667788);

    free(fb);
    return 0;
}
