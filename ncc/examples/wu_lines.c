#include <assert.h>
#include <stdlib.h>
#include <uvm/graphics.h>
#include <uvm/syscalls.h>
#include <uvm/utils.h>


size_t FRAME_WIDTH = 400;
size_t FRAME_HEIGHT = 400;

u32 frame_buffer[160000];

// Current mouse pointer position
size_t pos_x = 200;
size_t pos_y = 200;

#define RED_OF(color) ((color >> 16) & 255)
#define GREEN_OF(color) ((color >> 8) & 255)
#define BLUE_OF(color) (color & 255)

void
carefree_alpha_blend_plot_pixel(u32* dest, size_t dest_stride, u64 x, u64 y, u32 color, u8 alpha)
{
    if (!alpha) return;
    u32* pix_ptr = dest + dest_stride * y + x;
    if (0xFF == alpha)
    {
        *pix_ptr = color;
        return;
    }
    u32 dest_pixel = *pix_ptr;

    u8 unalpha = 0xFF - alpha;

    u8 red   = (  RED_OF(dest_pixel) * unalpha +   RED_OF(color) * alpha) / 0xff;
    u8 green = (GREEN_OF(dest_pixel) * unalpha + GREEN_OF(color) * alpha) / 0xff;
    u8 blue  = ( BLUE_OF(dest_pixel) * unalpha +  BLUE_OF(color) * alpha) / 0xff;

    *pix_ptr = rgb32(red, green, blue);
}


// > the “error adjust,” is stored as a fixed-point fraction, in 0.16 format (that is, all bits are fractional, and the decimal point is just to the left of bit 15). An error accumulator, also in 0.16 format, is initialized to 0. Then the first pixel is drawn; no weighting is needed, because the line intersects its endpoints exactly.

// LIMIT is 16 bits of 0.999... in 0.16 format
#define LIMIT 0xffff

// Draw a line using Wu's algorithm
void draw_wu_line(u32* fb, u32 fb_width, u32 fb_height, u32 x0, u32 y0, u32 x1, u32 y1, u32 color)
{
    if (x1 == x0)
    {
        if (y1 == y0)
        {
            //draw_point
            *(fb + x0 + y0 * fb_width) = color;
        }
        else
        {
            //draw_vertical_line
            if (y0 > y1)
            {
                // swap values
                //u32 tmp = y1; y1 = y0; y0 = tmp;
                // Use the xor trick!
                y1 = y1 ^ y0;
                y0 = y0 ^ y1;
                y1 = y1 ^ y0;
            }
            u32 *point = fb + x0;
            for (; y1 >= y0; ++y0)
            {
                *point = color;
                point = point + fb_width;
            }
        }
    }
    else if (y1 == y0)  // and we know x1 != x0
    {
        //draw_horizontal_line
    }
    else
    {
        u32 dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
        u32 dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
        if (dx == dy)
        {
            //draw_45_degree_line
        }
        else
        {
            if ((x1 > x0) == (y1 > y0))
            {
                if (dx > dy)
                {
                    if (x1 > x0) {
                        draw_wu_line_first_octant(fb, fb_width, fb_height, x0, y0, x1, y1, color);
                    } else {
                        draw_wu_line_first_octant(fb, fb_width, fb_height, x1, y1, x0, y0, color);
                    }
                }
                else // if (dy > dx)
                {
                    if (x1 > x0) {
                        draw_wu_line_second_octant(fb, fb_width, fb_height, x0, y0, x1, y1, color);
                    } else {
                        draw_wu_line_second_octant(fb, fb_width, fb_height, x1, y1, x0, y0, color);
                    }
                }
            }
            else  // signs differ
            {
                if (dx > dy)
                {
                    if (x1 > x0) {
                        draw_wu_line_third_octant(fb, fb_width, fb_height, x0, y0, x1, y1, color);
                    } else {
                        draw_wu_line_third_octant(fb, fb_width, fb_height, x1, y1, x0, y0, color);
                    }
                }
                else // if (dy > dx)
                {
                    if (x1 > x0) {
                        draw_wu_line_fourth_octant(fb, fb_width, fb_height, x1, y1, x0, y0, color);
                    } else {
                        draw_wu_line_fourth_octant(fb, fb_width, fb_height, x0, y0, x1, y1, color);
                    }
                }
            }
        }
    }
}

void draw_wu_line_first_octant(u32* fb, u32 fb_width, u32 fb_height, u32 x0, u32 y0, u32 x1, u32 y1, u32 color)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    assert(dx > 0 && dy > 0 && dx > dy);
    int error_adjust = LIMIT * dy / dx;
    int error_accumulator = 0;
    while (x1 >= x0)
    {
        u8 intensity = error_accumulator >> 8 & 0xff;
        carefree_alpha_blend_plot_pixel(fb, fb_width, x0, y0,     color, 0xFF - intensity);
        carefree_alpha_blend_plot_pixel(fb, fb_width, x0, y0 + 1, color, intensity);
        carefree_alpha_blend_plot_pixel(fb, fb_width, x1, y1,     color, 0xFF - intensity);
        carefree_alpha_blend_plot_pixel(fb, fb_width, x1, y1 - 1, color, intensity);
        ++x0;
        --x1;
        error_accumulator = error_accumulator + error_adjust;
        if (error_accumulator > LIMIT)
        {
            error_accumulator = error_accumulator - LIMIT;
            ++y0;
            --y1;
        }
    }
}

void draw_wu_line_second_octant(u32* fb, u32 fb_width, u32 fb_height, u32 x0, u32 y0, u32 x1, u32 y1, u32 color)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    assert(dx > 0 && dy > 0 && dy > dx);
    int error_adjust = LIMIT * dx / dy;
    int error_accumulator = 0;
    while (y1 >= y0)
    {
        u8 intensity = error_accumulator >> 8 & 0xff;
        carefree_alpha_blend_plot_pixel(fb, fb_width, x0, y0,     color, 0xFF - intensity);
        carefree_alpha_blend_plot_pixel(fb, fb_width, x0 + 1, y0, color, intensity);
        carefree_alpha_blend_plot_pixel(fb, fb_width, x1, y1,     color, 0xFF - intensity);
        carefree_alpha_blend_plot_pixel(fb, fb_width, x1 - 1, y1, color, intensity);
        ++y0;
        --y1;
        error_accumulator = error_accumulator + error_adjust;
        if (error_accumulator > LIMIT)
        {
            error_accumulator = error_accumulator - LIMIT;
            ++x0;
            --x1;
        }
    }
}

void draw_wu_line_third_octant(u32* fb, u32 fb_width, u32 fb_height, u32 x0, u32 y0, u32 x1, u32 y1, u32 color)
{
    int dx = x1 - x0;
    int dy = y0 - y1;
    assert(dx > 0 && dy > 0 && dx > dy);
    int error_adjust = LIMIT * dy / dx;
    int error_accumulator = 0;
    while (x1 >= x0)
    {
        u8 intensity = error_accumulator >> 8 & 0xff;
        carefree_alpha_blend_plot_pixel(fb, fb_width, x0, y0,     color, 0xFF - intensity);
        carefree_alpha_blend_plot_pixel(fb, fb_width, x0, y0 - 1, color, intensity);
        carefree_alpha_blend_plot_pixel(fb, fb_width, x1, y1,     color, 0xFF - intensity);
        carefree_alpha_blend_plot_pixel(fb, fb_width, x1, y1 + 1, color, intensity);
        ++x0;
        --x1;
        error_accumulator = error_accumulator + error_adjust;
        if (error_accumulator > LIMIT)
        {
            error_accumulator = error_accumulator - LIMIT;
            ++y1;
            --y0;
        }
    }
}

void draw_wu_line_fourth_octant(u32* fb, u32 fb_width, u32 fb_height, u32 x0, u32 y0, u32 x1, u32 y1, u32 color)
{
    int dx = x0 - x1;
    int dy = y1 - y0;
    assert(dx > 0 && dy > 0 && dy > dx);
    int error_adjust = LIMIT * dx / dy;
    int error_accumulator = 0;
    while (y1 >= y0)
    {
        u8 intensity = error_accumulator >> 8 & 0xff;
        carefree_alpha_blend_plot_pixel(fb, fb_width, x0, y0,     color, 0xFF - intensity);
        carefree_alpha_blend_plot_pixel(fb, fb_width, x0 - 1, y0, color, intensity);
        carefree_alpha_blend_plot_pixel(fb, fb_width, x1, y1,     color, 0xFF - intensity);
        carefree_alpha_blend_plot_pixel(fb, fb_width, x1 + 1, y1, color, intensity);
        ++y0;
        --y1;
        error_accumulator = error_accumulator + error_adjust;
        if (error_accumulator > LIMIT)
        {
            error_accumulator = error_accumulator - LIMIT;
            --x0;
            ++x1;
        }
    }
}


void
mousemove(u64 window_id, u64 new_x, u64 new_y)
{
    // Update the mouse position
    pos_x = new_x;
    pos_y = new_y;
}


void anim_callback()
{
    // Grey background.
    memset(frame_buffer, 0x7f, sizeof(frame_buffer));

    u32 w = FRAME_WIDTH - 1;
    u32 h = FRAME_HEIGHT - 1;
    draw_wu_line(frame_buffer, FRAME_WIDTH, FRAME_HEIGHT, pos_x, pos_y, 0, 0, COLOR_RED);
    draw_wu_line(frame_buffer, FRAME_WIDTH, FRAME_HEIGHT, pos_x, pos_y, w, h, COLOR_GREEN);
    draw_wu_line(frame_buffer, FRAME_WIDTH, FRAME_HEIGHT, pos_x, pos_y, 0, h, COLOR_YELLOW);
    draw_wu_line(frame_buffer, FRAME_WIDTH, FRAME_HEIGHT, pos_x, pos_y, w, 0, COLOR_BLUE);
    draw_wu_line(frame_buffer, FRAME_WIDTH, FRAME_HEIGHT, h - pos_y, pos_x, 0, 0,  COLOR_GREEN);
    draw_wu_line(frame_buffer, FRAME_WIDTH, FRAME_HEIGHT, h - pos_y, pos_x, w, h,COLOR_RED );
    draw_wu_line(frame_buffer, FRAME_WIDTH, FRAME_HEIGHT, h - pos_y, pos_x, 0, h, COLOR_BLUE);
    draw_wu_line(frame_buffer, FRAME_WIDTH, FRAME_HEIGHT, h - pos_y, pos_x, w, 0, COLOR_YELLOW );

    window_draw_frame(0, frame_buffer);

    time_delay_cb(10, anim_callback);
}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Wu Anti-Aliased Line Example", 0);

    window_on_mousemove(0, mousemove);

    time_delay_cb(0, anim_callback);

    enable_event_loop();
}
