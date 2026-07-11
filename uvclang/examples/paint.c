// A minimal paint program: drag the left mouse button to draw, click a swatch
// in the bottom palette to pick a color.

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <uvm/syscalls.h>
#include <uvm/window.h>

#define FRAME_WIDTH 800
#define FRAME_HEIGHT 600
#define NUM_COLORS 32
#define BOX_WIDTH 25
#define BOX_HEIGHT 25
#define BRUSH_RADIUS 4

// 2D RGBA pixel array
uint32_t frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

// Current mouse pointer position
size_t pos_x = 200;
size_t pos_y = 200;

// Current color to draw with
uint32_t brush_color = 0xFF0000;

// Are we currently drawing?
bool drawing = false;

// Fill a rectangle area of pixels in a frame buffer
void fill_rect(
    uint32_t *f_buffer,
    size_t f_width,
    size_t f_height,
    size_t r_x,
    size_t r_y,
    size_t r_width,
    size_t r_height,
    uint32_t color
)
{
    for (size_t j = 0; j < r_height; ++j)
    {
        for (size_t i = 0; i < r_width; ++i)
        {
            uint32_t *pix_ptr = f_buffer + f_width * (r_y + j) + (r_x + i);
            *pix_ptr = color;
        }
    }
}

void draw_brush(void)
{
    size_t xmin = pos_x - BRUSH_RADIUS;
    size_t xmax = pos_x + BRUSH_RADIUS;

    size_t ymin = pos_y - BRUSH_RADIUS;
    size_t ymax = pos_y + BRUSH_RADIUS;

    if (xmax > FRAME_WIDTH) xmax = FRAME_WIDTH;
    if (ymax >= FRAME_HEIGHT - BOX_HEIGHT) ymax = FRAME_HEIGHT - BOX_HEIGHT;

    for (size_t x = xmin; x < xmax; ++x)
    {
        for (size_t y = ymin; y < ymax; ++y)
        {
            size_t dx = x - pos_x;
            size_t dy = y - pos_y;
            size_t dist_sqr = dx * dx + dy * dy;

            if (dist_sqr > BRUSH_RADIUS * BRUSH_RADIUS)
                continue;

            uint32_t *pix_ptr = (uint32_t *)frame_buffer + (FRAME_WIDTH * y + x);
            *pix_ptr = brush_color;
        }
    }
}

// Get a pointer to the pixel data at a given position
// so that we can read the current pixel color there
uint32_t *get_pixel_ptr(
    uint32_t *f_buffer,
    size_t f_width,
    size_t f_height,
    size_t x,
    size_t y
)
{
    return f_buffer + (f_width * y) + x;
}

void draw_palette(void)
{
    for (size_t i = 0; i < NUM_COLORS; ++i)
    {
        // Each component is 127 * i where i is 0, 1, 2
        // R color = (i % 3) * 127
        // R color = ((i/3) % 3) * 127
        // G color = ((i/9) % 3) * 127
        // Add an offset so that black doesn't end up right at the end
        size_t color_idx = i + 3;
        uint32_t r = (color_idx % 3) * 127;
        uint32_t g = ((color_idx / 3) % 3) * 127;
        uint32_t b = ((color_idx / 9) % 3) * 127;
        uint32_t color = (r << 16) | (g << 8) | b;

        size_t xmin = i * BOX_WIDTH;
        size_t ymin = FRAME_HEIGHT - BOX_HEIGHT;

        fill_rect(
            (uint32_t *)frame_buffer,
            FRAME_WIDTH,
            FRAME_HEIGHT,
            xmin,
            ymin,
            BOX_WIDTH,
            BOX_HEIGHT,
            color
        );
    }
}

// Mouse movement callback
void mousemove(int new_x, int new_y)
{
    if (drawing)
    {
        int64_t dx = new_x - pos_x;
        int64_t dy = new_y - pos_y;

        int64_t num_steps = 1;
        if (llabs(dx) > 3)
            num_steps = llabs(dx) / 3;
        if (llabs(dy) > llabs(dx))
            num_steps = llabs(dy) / 3;

        for (int64_t i = 0; i < num_steps; ++i)
        {
            pos_x = pos_x + (dx / num_steps);
            pos_y = pos_y + (dy / num_steps);
            draw_brush();
        }
    }

    // Update the brush position
    pos_x = new_x;
    pos_y = new_y;

    window_draw_frame(0, (uint8_t *)frame_buffer);
}

void mousedown(uint16_t btn_id)
{
    if (btn_id == 0)
    {
        if (pos_y > FRAME_HEIGHT - BOX_HEIGHT)
        {
            uint32_t *pixel_ptr = get_pixel_ptr((uint32_t *)frame_buffer, FRAME_WIDTH, FRAME_HEIGHT, pos_x, pos_y);
            brush_color = *pixel_ptr;
        }
        else
        {
            drawing = true;
            draw_brush();
        }
    }

    window_draw_frame(0, (uint8_t *)frame_buffer);
}

void mouseup(uint16_t btn_id)
{
    if (btn_id == 0)
    {
        drawing = false;
    }
}

Event event;

int main(void)
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "UVM Paint Program Example", 0);

    // Initially fill the canvas with white
    fill_rect(
        (uint32_t *)frame_buffer,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        0,
        0,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        0xFFFFFF
    );

    draw_palette();

    window_draw_frame(0, (uint8_t *)frame_buffer);

    for (;;)
    {
        window_wait_event(&event);

        if (event.kind == EVENT_QUIT)
        {
            exit(0);
        }

        if (event.kind == EVENT_MOUSEDOWN)
        {
            mousedown(event.button);
        }

        if (event.kind == EVENT_MOUSEUP)
        {
            mouseup(event.button);
        }

        if (event.kind == EVENT_MOUSEMOVE)
        {
            mousemove(event.x, event.y);
        }
    }

    return 0;
}
