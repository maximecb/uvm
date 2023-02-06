#include <assert.h>

size_t FRAME_WIDTH = 800;
size_t FRAME_HEIGHT = 600;
u32 DOT_SIZE = 8;
u32 CH_DOTS_X = 5;
u32 CH_DOTS_Y = 7;

// RGB pixels: 800 * 600 * 3
u8 FRAME_BUFFER[1_440_000];

// Strings mapping the dots for each character
char* CHAR_DOTS[256] = 0;

u64 start_time = 0;

void init_dots()
{
    CHAR_DOTS['0'] = (
        " *** "
        "*   *"
        "*   *"
        "*   *"
        "*   *"
        "*   *"
        " *** "
    );

    CHAR_DOTS['1'] = (
        "  *  "
        " **  "
        "  *  "
        "  *  "
        "  *  "
        "  *  "
        " *** "
    );

    CHAR_DOTS['2'] = (
        " *** "
        "*   *"
        "   * "
        "  *  "
        " *   "
        "*    "
        "*****"
    );

    CHAR_DOTS['3'] = (
        " *** "
        "*   *"
        "    *"
        "  ***"
        "    *"
        "*   *"
        " *** "
    );

    CHAR_DOTS['4'] = (
        "*   *"
        "*   *"
        "*   *"
        "*****"
        "    *"
        "    *"
        "    *"
    );

    CHAR_DOTS['5'] = (
        "*****"
        "*    "
        "*    "
        "**** "
        "    *"
        "    *"
        "**** "
    );

    CHAR_DOTS['6'] = (
        " *** "
        "*   *"
        "*    "
        "**** "
        "*   *"
        "*   *"
        " *** "
    );

    CHAR_DOTS['7'] = (
        "*****"
        "    *"
        "   * "
        "  *  "
        "  *  "
        "  *  "
        "  *  "
    );

    CHAR_DOTS['8'] = (
        "*****"
        "*   *"
        "*   *"
        "*****"
        "*   *"
        "*   *"
        "*****"
    );

    CHAR_DOTS['9'] = (
        "*****"
        "*   *"
        "*   *"
        "*****"
        "    *"
        "    *"
        "*****"
    );
}

void draw_circle(int xmin, int ymin, int size)
{
    int xmax = xmin + size;
    int ymax = ymin + size;
    int radius = size / 2;
    int cx = xmin + radius;
    int cy = ymin + radius;
    int r2 = (radius - 1) * (radius - 1);

    for (int y = ymin; y < ymax; ++y)
    {
        for (int x = xmin; x < xmax; ++x)
        {
            //print_i64(x);
            //print_endl();

            //print_i64(y);
            //print_endl();

            int dx = x - cx;
            int dy = y - cy;
            int dist_sqr = (dx * dx) + (dy * dy);

            if (dist_sqr > r2)
                continue;

            u8* pix_ptr = FRAME_BUFFER + (3 * FRAME_WIDTH) * y + (3 * x);
            *(pix_ptr + 0) = 255;
            *(pix_ptr + 1) = 0;
            *(pix_ptr + 2) = 0;
        }
    }
}

void draw_char(int xmin, int ymin, int dot_size, char ch)
{
    char* dots = CHAR_DOTS[ch];
    assert(dots);

    for (int j = 0; j < CH_DOTS_Y; ++j)
    {
        for (int i = 0; i < CH_DOTS_X; ++i)
        {
            bool dot_active = dots[j * CH_DOTS_X + i] == '*';

            if (!dot_active)
                continue;

            int x = xmin + i * DOT_SIZE;
            int y = ymin + j * DOT_SIZE;
            draw_circle(x, y, dot_size);
        }
    }
}

void draw_number(int xmax, int ymin, int dot_size, int number)
{
    int num_digits = 0;
    for (int n = number; n > 0; n = n / 10)
    {
        ++num_digits;
    }

    for (int i = 0; i < num_digits; ++i)
    {
        int digit = number % 10;
        number = number / 10;

        draw_char(
            xmax - 50 * i,
            ymin,
            dot_size,
            '0' + digit
        );
    }
}

void anim_callback()
{
    // Clear the screen
    memset(FRAME_BUFFER, 0, 1_440_000);

    u64 delta_time = time_current_ms() - start_time;
    u64 seconds = delta_time / 10;
    int s = asm (seconds) -> int {};

    draw_number(500, 200, 10, s);

    window_draw_frame(0, FRAME_BUFFER);
    time_delay_cb(50, anim_callback);
}

void main()
{
    init_dots();

    start_time = time_current_ms();

    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Counter", 0);
    window_show(0);

    time_delay_cb(0, anim_callback);
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

// Fill a block of bytes in the heap with a given value.
inline void memset(u8* dst, u8 value, u64 num_bytes)
{
    return asm (dst, value, num_bytes) -> void { syscall 4; };
}

// Get the UNIX time stamp in milliseconds.
inline u64 time_current_ms()
{
    return asm () -> u64 { syscall 0; };
}

// Schedule a callback to be called once after a given delay.
inline void time_delay_cb(u64 delay_ms, void* callback)
{
    return asm (delay_ms, callback) -> void { syscall 2; };
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
