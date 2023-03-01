#ifndef __UVM_GRAPHICS__
#define __UVM_GRAPHICS__

#define COLOR_BLACK     0xFF_00_00_00
#define COLOR_WHITE     0xFF_FF_FF_FF
#define COLOR_RED       0xFF_FF_00_00
#define COLOR_GREEN     0xFF_00_FF_00
#define COLOR_BLUE      0xFF_00_00_FF
#define COLOR_ORANGE    0xFF_FF_A5_00
#define COLOR_YELLOW    0xFF_FF_FF_00
#define COLOR_MAGENTA   0xFF_FF_00_FF

// Draw a line using Bresenham's algorithm
void draw_line(u32* fb, u32 fb_width, u32 fb_height, u32 x0, u32 y0, u32 x1, u32 y1, u32 color)
{
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
        dy = y1 - y0;
        sy = 1;
    }
    else
    {
        dy = y0 - y1;
        sy = -1;
    }

    int err = dx + dy;

	for (;;)
    {
        if (x0 < fb_width && y0 < fb_height)
        {
            u32* pix_ptr = fb + y0 * fb_width + x0;
            *pix_ptr = color;
        }

		if(x0 == x1 && y0 == y1)
			break;

		int e2 = 2 * err;

		if(e2 >= dy)
        {
			err = err + dy;
			x0 = x0 + sx;
		}

		if(e2 <= dx)
        {
			err = err + dx;
			y0 = y0 + sy;
		}
	}
}

#endif
