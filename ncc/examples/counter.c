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
        " ** "
        "  *  "
        "  *  "
        "  *  "
        "  *  "
        "  *  "
        " *** "
    );

    char* dots = CHAR_DOTS['0'];
    assert(dots);




}

void draw_circle(int xmin, int ymin, int size)
{
    int xmax = xmin + size;
    int ymax = ymin + size;
    int cx = xmin + size / 2;
    int cy = ymin + size / 2;






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

            if (dist_sqr <= size * size)
            {
                u8* pix_ptr = FRAME_BUFFER + (3 * FRAME_WIDTH) * y + (3 * x);
                *(pix_ptr + 0) = 255;
                *(pix_ptr + 1) = 0;
                *(pix_ptr + 2) = 0;
            }
        }
    }
}

void draw_char(int xmin, int ymin, int dot_size, char ch)
{
    //print_i64('0');
    //print_endl();

    //print_i64(ch);
    //print_endl();

    //char* dots = CHAR_DOTS[ch];
    //asm (dots) -> void { syscall 5; };
    //print_endl();

    char* dots = CHAR_DOTS['0'];
    assert(dots);
    print_str(dots);
    print_endl();

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





void anim_callback()
{
    // Clear the screen
    memset(FRAME_BUFFER, 0, 1_440_000);



    draw_char(100, 100, 10, '0');






    time_delay_cb(10, anim_callback);
}

void main()
{
    init_dots();

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

// Print an i64 value to standard output
inline void print_i64(i64 val)
{
    return asm (val) -> void { syscall 5; };
}

// Print a string to standard output
inline void print_str(char* str)
{
    return asm (str) -> void { syscall 6; };
}

// Print a newline to standard output
inline void print_endl()
{
    return asm () -> void { syscall 7; };
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
