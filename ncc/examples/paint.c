size_t WIDTH = 800;
size_t HEIGHT = 600;
size_t NUM_COLORS = 27;

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
    for (size_t j = 0; j < r_height; j = j + 1)
    {
        for (size_t i = 0; i < r_width; i = i + 1)
        {
            u8* pix_addr = f_buffer + (3 * f_width) * (r_y + j) + 3 * (r_x + i);
            *(pix_addr + 0) = r;
            *(pix_addr + 1) = g;
            *(pix_addr + 2) = b;
        }
    }
}

void draw_colors()
{
    for (size_t i = 0; i < NUM_COLORS; i = i + 1)
    {
        // Each component is 127 * i where i is 0, 1, 2
        // R color = (i % 3) * 127
        // R color = ((i/3) % 3) * 127
        // G color = ((i/9) % 3) * 127
        u8 r = (i % 3) * 127;
        u8 g = ((i/3) % 3) * 127;
        u8 b = ((i/9) % 3) * 127;






    }
}

void main()
{
}
