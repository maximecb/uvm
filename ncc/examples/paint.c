//char* WINDOW_TITLE = "UVM Paint Program Demo";

size_t FRAME_WIDTH = 800;
size_t FRAME_HEIGHT = 600;
size_t NUM_COLORS = 27;
size_t BOX_WIDTH = 29;
size_t BOX_HEIGHT = 29;
size_t POINTER_SIZE = 10;

u8* f_buffer = null;

// Current mouse pointer position
size_t pos_x = 200;
size_t pos_y = 200;

// Current color to draw with
u8 current_r = 0;
u8 current_g = 0;
u8 current_b = 0;

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
    return f_buffer + (3 * f_width) * y + 3 * x;
}

void draw_colors()
{
    // Initially fill the canvas with white
    fill_rect(
        f_buffer,
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
            f_buffer,
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

void main()
{
    // TODO: call to create window

    draw_colors();

    // TODO: try drawing the mouse pointer the current color (see globals)


    // TODO: call to copy pixels
}
