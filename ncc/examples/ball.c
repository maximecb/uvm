char* WINDOW_TITLE = "Bouncing Ball Demo";

size_t FRAME_WIDTH = 800;
size_t FRAME_HEIGHT = 600;
size_t NUM_COLORS = 32;
size_t BALL_RADIUS = 20;

// RGB pixels: 800 * 600 * 3
u8 FRAME_BUFFER[1_440_000];

// Current ball position
u64 px = 200;
u64 py = 200;

// Velocity
u64 vx = 5;
u64 vy = 7;

// Draw the ball at the current x,y position
void draw_ball()
{
    size_t xmin = px;
    size_t xmax = px + BALL_RADIUS;
    if (px >= BALL_RADIUS) xmin = px - BALL_RADIUS;
    if (xmax >= FRAME_WIDTH) xmax = FRAME_WIDTH;

    size_t ymin = py;
    size_t ymax = py + BALL_RADIUS;
    if (py >= BALL_RADIUS) ymin = py - BALL_RADIUS;
    if (ymax >= FRAME_HEIGHT) ymax = FRAME_HEIGHT;

    for (size_t x = xmin; x < xmax; ++x)
    {
        for (size_t y = ymin; y < ymax; ++y)
        {
            size_t dx = x - px;
            size_t dy = y - py;
            size_t dist_sqr = dx * dx + dy * dy;

            if (dist_sqr <= BALL_RADIUS * BALL_RADIUS)
            {
                u8* pix_ptr = FRAME_BUFFER + (3 * FRAME_WIDTH) * y + 3 * x;
                *(pix_ptr + 0) = 255;
                *(pix_ptr + 1) = 0;
                *(pix_ptr + 2) = 0;
            }
        }
    }
}

void anim_callback()
{
    // Clear the screen
    memset(FRAME_BUFFER, 0, 1_440_000);

    draw_ball();

    px = px + vx;
    py = py + vy;

    if (px + BALL_RADIUS > FRAME_WIDTH)
    {
        vx = -vx;
    }
    if (px - BALL_RADIUS < 0)
    {
        vx = -vx;
    }

    if (py + BALL_RADIUS > FRAME_HEIGHT)
    {
        vy = -vy;
    }
    if (py - BALL_RADIUS < 0)
    {
        vy = -vy;
    }

    window_draw_frame(0, FRAME_BUFFER);

    time_delay_cb(10, anim_callback);
}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, WINDOW_TITLE, 0);

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
