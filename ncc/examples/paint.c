char* WINDOW_TITLE = "UVM Paint Program Demo";

size_t FRAME_WIDTH = 800;
size_t FRAME_HEIGHT = 600;
size_t NUM_COLORS = 32;
size_t BOX_WIDTH = 25;
size_t BOX_HEIGHT = 25;
size_t BRUSH_RADIUS = 4;

// RGB pixels: 800 * 600 * 3
u8 FRAME_BUFFER[1_440_000];

// Current mouse pointer position
size_t pos_x = 200;
size_t pos_y = 200;

// Current color to draw with
u8 current_r = 255;
u8 current_g = 0;
u8 current_b = 0;

// Are we currently drawing?
bool drawing = false;

// Fill a rectangle area of pixels in a frame buffer
void fill_rect(
    u8* f_buffer,
    size_t f_width,
    size_t f_height,
    size_t r_x,
    size_t r_y,
    size_t r_width,
    size_t r_height,
    u8 r,
    u8 g,
    u8 b
)
{
    for (size_t j = 0; j < r_height; ++j)
    {
        for (size_t i = 0; i < r_width; ++i)
        {
            u8* pix_addr = f_buffer + (3 * f_width) * (r_y + j) + 3 * (r_x + i);
            *(pix_addr + 0) = r;
            *(pix_addr + 1) = g;
            *(pix_addr + 2) = b;
        }
    }
}

void draw_brush()
{
    size_t xmin = pos_x;
    size_t xmax = pos_x + BRUSH_RADIUS;
    if (pos_x >= BRUSH_RADIUS) xmin = pos_x - BRUSH_RADIUS;
    if (xmax >= FRAME_WIDTH) xmax = FRAME_WIDTH;

    size_t ymin = pos_y;
    size_t ymax = pos_y + BRUSH_RADIUS;
    if (pos_y >= BRUSH_RADIUS) ymin = pos_y - BRUSH_RADIUS;
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

            u8* pix_ptr = FRAME_BUFFER + (3 * FRAME_WIDTH) * y + 3 * x;
            *(pix_ptr + 0) = current_r;
            *(pix_ptr + 1) = current_g;
            *(pix_ptr + 2) = current_b;
        }
    }
}

/// Get a pointer to the pixel data at a given position
/// so that we can read the current pixel color there
u8* get_pixel_ptr(
    u8* f_buffer,
    size_t f_width,
    size_t f_height,
    size_t x,
    size_t y,
)
{
    return f_buffer + (3 * f_width * y) + (3 * x);
}

void create_window(char* window_title, size_t width, size_t height)
{
    asm (width, height, window_title) -> void
    {
        syscall window_create;
        syscall window_show;
    };
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
        u8 r = (color_idx % 3) * 127;
        u8 g = ((color_idx/3) % 3) * 127;
        u8 b = ((color_idx/9) % 3) * 127;

        size_t xmin = i * BOX_WIDTH;
        size_t ymin = FRAME_HEIGHT - BOX_HEIGHT;

        fill_rect(
            FRAME_BUFFER,
            FRAME_WIDTH,
            FRAME_HEIGHT,
            xmin,
            ymin,
            BOX_WIDTH,
            BOX_HEIGHT,
            r,
            g,
            b
        );
    }
}

// Mouve movement callback
void mousemove(u64 window_id, u64 x, u64 y)
{
    // Update the brush position
    pos_x = x;
    pos_y = y;

    if (drawing) {
        draw_brush();
    }

    asm (FRAME_BUFFER) -> void
    {
        syscall window_copy_pixels;
    };
}

void mousedown(u64 window_id, u8 btn_id)
{
    if (btn_id == 0) {
        drawing = true;
        draw_brush();
    }

    asm (btn_id) -> void
    {
        syscall print_i64;
        syscall print_endl;
    };

    if (btn_id == 2) {
        u8* pixel_ptr = get_pixel_ptr(FRAME_BUFFER, FRAME_WIDTH, FRAME_HEIGHT, pos_x, pos_y);
        current_r = *(pixel_ptr + 0);
        current_g = *(pixel_ptr + 1);
        current_b = *(pixel_ptr + 2);
    }

    asm (FRAME_BUFFER) -> void
    {
        syscall window_copy_pixels;
    };
}

void mouseup(u64 window_id, u8 btn_id)
{
    if (btn_id == 0) {
        drawing = false;
    }
}

void main()
{
    // TODO: call to create window
    create_window(WINDOW_TITLE, FRAME_WIDTH, FRAME_HEIGHT);

    // Initially fill the canvas with white
    fill_rect(
        FRAME_BUFFER,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        0,
        0,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        255,
        255,
        255
    );

    draw_palette();

    // Register mouse event callbacks
    asm (0, mousemove) -> void { syscall window_on_mousemove; };
    asm (0, mousedown) -> void { syscall window_on_mousedown; };
    asm (0, mouseup) -> void { syscall window_on_mouseup; };

    asm (FRAME_BUFFER) -> void
    {
        syscall window_copy_pixels;
    };

    __enable_event_loop__();
}

// Function to enable returning to the event loop instead of exiting
void __enable_event_loop__()
{
    asm () -> void
    {
        push __EVENT_LOOP_ENABLED__;
        push 1;
        store_u8;
    };
}
