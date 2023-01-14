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

void create_window(char* window_title, size_t width, size_t height)
{
    asm (width, height, window_title) -> void
    {
        syscall window_create;
        syscall window_show;
    };
}

void anim_callback()
{
    u64 start_time_ms = asm () -> u64 { syscall time_current_ms; };

    // Clear the screen
    asm (FRAME_BUFFER, 0, 1_440_000) -> void { syscall memset; };

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

    asm (FRAME_BUFFER) -> void { syscall window_copy_pixels; };

    asm (10, anim_callback) -> void { syscall time_delay_cb; };

    u64 end_time_ms = asm () -> u64 { syscall time_current_ms; };
    u64 delta_time = end_time_ms - start_time_ms;

    asm ("ms", delta_time) -> u64 {
        syscall print_i64;
        syscall print_str;
        syscall print_endl;
    };
}

void main()
{
    // TODO: call to create window
    create_window(WINDOW_TITLE, FRAME_WIDTH, FRAME_HEIGHT);

    asm (0, anim_callback) -> void { syscall time_delay_cb; };

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
