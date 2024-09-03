#include <uvm/syscalls.h>
#include <uvm/window.h>
#include <stdlib.h>

#define FRAME_WIDTH 800
#define FRAME_HEIGHT 600
#define NUM_COLORS 32
#define BOX_WIDTH 25
#define BOX_HEIGHT 25
#define BRUSH_RADIUS 4

// 2D RGBA pixel array
u32 frame_buffer[FRAME_HEIGHT][FRAME_WIDTH];

// Current mouse pointer position
size_t pos_x = 200;
size_t pos_y = 200;

// Current color to draw with
u32 brush_color = 0xFF_00_00;

// Are we currently drawing?
bool drawing = false;

// Fill a rectangle area of pixels in a frame buffer
void fill_rect(
    u32* f_buffer,
    size_t f_width,
    size_t f_height,
    size_t r_x,
    size_t r_y,
    size_t r_width,
    size_t r_height,
    u32 color,
)
{
    for (size_t j = 0; j < r_height; ++j)
    {
        for (size_t i = 0; i < r_width; ++i)
        {
            u32* pix_ptr = f_buffer + f_width * (r_y + j) + (r_x + i);
            *pix_ptr = color;
        }
    }
}

void draw_brush()
{
    size_t xmin = pos_x - BRUSH_RADIUS;
    size_t xmax = pos_x + BRUSH_RADIUS;

    size_t ymin = pos_y - BRUSH_RADIUS;
    size_t ymax = pos_y + BRUSH_RADIUS;

    if (xmin < 0) xmin = 0;
    if (xmax > FRAME_WIDTH) xmax = FRAME_WIDTH;

    if (ymin < 0) ymin = 0;
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

            u32* pix_ptr = (u32*)frame_buffer + (FRAME_WIDTH * y + x);
            *pix_ptr = brush_color;
        }
    }
}

/// Get a pointer to the pixel data at a given position
/// so that we can read the current pixel color there
u32* get_pixel_ptr(
    u32* f_buffer,
    size_t f_width,
    size_t f_height,
    size_t x,
    size_t y,
)
{
    return f_buffer + (f_width * y) + x;
}

void draw_palette()
{
    for (size_t i = 0; i < NUM_COLORS; ++i)
    {
        // Each component is 127 * i where i is 0, 1, 2
        // R color = (i % 3) * 127
        // R color = ((i/3) % 3) * 127
        // G color = ((i/9) % 3) * 127
        // Add an offset so that black doesn't end up right at the end
        size_t color_idx = i + 3;
        u32 r = (color_idx % 3) * 127;
        u32 g = ((color_idx/3) % 3) * 127;
        u32 b = ((color_idx/9) % 3) * 127;
        u32 color = (r << 16) | (g << 8) | b;

        size_t xmin = i * BOX_WIDTH;
        size_t ymin = FRAME_HEIGHT - BOX_HEIGHT;

        fill_rect(
            (u32*)frame_buffer,
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

// Mouve movement callback
void mousemove(int new_x, int new_y)
{
    if (drawing)
    {
        i64 dx = new_x - pos_x;
        i64 dy = new_y - pos_y;

        i64 num_steps = 1;
        if (abs(dx) > 3)
            num_steps = abs(dx) / 3;
        if (abs(dy) > abs(dx))
            num_steps = abs(dy) / 3;

        for (i64 i = 0; i < num_steps; ++i)
        {
            pos_x = pos_x + (dx / num_steps);
            pos_y = pos_y + (dy / num_steps);
            draw_brush();
        }
    }

    // Update the brush position
    pos_x = new_x;
    pos_y = new_y;

    window_draw_frame(0, frame_buffer);
}

void mousedown(u16 btn_id)
{
    if (btn_id == 0)
    {
        if (pos_y > FRAME_HEIGHT - BOX_HEIGHT)
        {
            u32* pixel_ptr = get_pixel_ptr((u32*)frame_buffer, FRAME_WIDTH, FRAME_HEIGHT, pos_x, pos_y);
            brush_color = *pixel_ptr;
        }
        else
        {
            drawing = true;
            draw_brush();
        }
    }

    window_draw_frame(0, frame_buffer);
}

void mouseup(u16 btn_id)
{
    if (btn_id == 0)
    {
        drawing = false;
    }
}

Event event;

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "UVM Paint Program Example", 0);

    // Initially fill the canvas with white
    fill_rect(
        (u32*)frame_buffer,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        0,
        0,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        0xFF_FF_FF
    );

    draw_palette();

    window_draw_frame(0, frame_buffer);

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
}
