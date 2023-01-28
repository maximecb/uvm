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

    window_draw_frame(0, FRAME_BUFFER);
}

void mousedown(u64 window_id, u8 btn_id)
{
    if (btn_id == 0) {
        drawing = true;
        draw_brush();
    }

    if (btn_id == 2) {
        u8* pixel_ptr = get_pixel_ptr(FRAME_BUFFER, FRAME_WIDTH, FRAME_HEIGHT, pos_x, pos_y);
        current_r = *(pixel_ptr + 0);
        current_g = *(pixel_ptr + 1);
        current_b = *(pixel_ptr + 2);
    }

    window_draw_frame(0, FRAME_BUFFER);
}

void mouseup(u64 window_id, u8 btn_id)
{
    print_str("mouseup!\n");

    if (btn_id == 0) {
        drawing = false;
    }
}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, WINDOW_TITLE, 0);
    window_show(0);

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
    window_on_mousemove(0, mousemove);
    window_on_mousedown(0, mousedown);
    window_on_mouseup(0, mouseup);

    window_draw_frame(0, FRAME_BUFFER);

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

// Print a string to standard output
inline void print_str(char* str)
{
    return asm (str) -> void { syscall 6; };
}

// Create a new window with a frame buffer to draw into.
inline u32 window_create(u32 width, u32 height, char* title, u64 flags)
{
    return asm (width, height, title, flags) -> u32 { syscall 1; };
}

// Show a window, initially not visible when created.
inline void window_show(u32 window_id)
{
    return asm (window_id) -> void { syscall 9; };
}

// Copy a frame of RGB24 pixels to be displayed into the window.
inline void window_draw_frame(u32 window_id, u8* pixel_data)
{
    return asm (window_id, pixel_data) -> void { syscall 10; };
}

// Register a callback for mouse movement.
inline void window_on_mousemove(u32 window_id, void* callback)
{
    return asm (window_id, callback) -> void { syscall 11; };
}

// Register a callback for mouse button press events.
inline void window_on_mousedown(u32 window_id, void* callback)
{
    return asm (window_id, callback) -> void { syscall 12; };
}

// Register a callback for mouse button release events.
inline void window_on_mouseup(u32 window_id, void* callback)
{
    return asm (window_id, callback) -> void { syscall 13; };
}
