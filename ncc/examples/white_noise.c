char* WINDOW_TITLE = "White Noise Demo";

size_t FRAME_WIDTH = 800;
size_t FRAME_HEIGHT = 600;

// RGB pixels: 800 * 600 * 3
u8 FRAME_BUFFER[1_440_000];

u64 cur_rand = 1337;

u32 rand()
{
    cur_rand = (0xf1357aea2e62a9c5 * cur_rand);
    return cur_rand >> 32;
}

void draw_frame()
{
    for (size_t y = 0; y < FRAME_HEIGHT; ++y)
    {
        if (y % 2 == 0)
        {
            memset(FRAME_BUFFER + 3 * FRAME_WIDTH * y, 150, 3 * FRAME_WIDTH);
        }
        else
        {
            for (size_t x = 0; x < FRAME_WIDTH; ++x)
            {
                u32 c = rand() % 256;

                u8* pix_ptr = FRAME_BUFFER + (3 * FRAME_WIDTH) * y + 3 * x;
                *(pix_ptr + 0) = c;
                *(pix_ptr + 1) = c;
                *(pix_ptr + 2) = c;
            }
        }
    }
}

void anim_callback()
{
    // Clear the screen
    //memset(FRAME_BUFFER, 0, 1_440_000);

    u64 start_time = time_current_ms();
    draw_frame();
    u64 end_time = time_current_ms();
    u64 dt = end_time - start_time;

    print_i64(dt);
    print_str("ms");
    print_endl();

    window_draw_frame(0, FRAME_BUFFER);

    time_delay_cb(10, anim_callback);
}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, WINDOW_TITLE, 0);
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
