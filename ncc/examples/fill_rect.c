// Fill a rectangle area of pixels in a frame buffer
void fill_rect(
    u8* f_buffer,
    size_t f_width,
    size_t f_height,
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
            u8* pix_addr = f_buffer + (j * r_width + i) * 3;
            *(pix_addr + 0) = r;
            *(pix_addr + 1) = g;
            *(pix_addr + 0) = b;
        }
    }
}

void main()
{
}
