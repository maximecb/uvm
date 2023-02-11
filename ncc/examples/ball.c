#include <uvm/syscalls.h>
#include <uvm/utils.h>

size_t FRAME_WIDTH = 800;
size_t FRAME_HEIGHT = 600;
size_t BALL_RADIUS = 20;

// RGBA pixels: 800 * 600
u32 FRAME_BUFFER[480_000];

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
                u32* pix_ptr = FRAME_BUFFER + (FRAME_WIDTH * y + x);
                *pix_ptr = 0xFF_00_00;
            }
        }
    }
}

void anim_callback()
{
    // Clear the screen
    memset(FRAME_BUFFER, 0, 1_920_000);

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
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Bouncing Ball Example", 0);

    time_delay_cb(0, anim_callback);

    enable_event_loop();
}
