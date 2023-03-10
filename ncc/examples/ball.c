#include <uvm/syscalls.h>
#include <uvm/utils.h>

#define FRAME_WIDTH 800
#define FRAME_HEIGHT 600
#define BALL_RADIUS 20

// RGBA pixels: 800 * 600
u32 frame_buffer[480_000];

// Current ball position
int px = 200;
int py = 200;

// Velocity
int vx = 5;
int vy = 7;

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
                u32* pix_ptr = frame_buffer + (FRAME_WIDTH * y + x);
                *pix_ptr = 0xFF_00_00;
            }
        }
    }
}

void anim_callback()
{
    u64 start_time = time_current_ms();

    // Clear the frame buffer, set all pixels to black
    memset32(frame_buffer, 0, 800 * 600);

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

    window_draw_frame(0, frame_buffer);

    // Schedule a fixed rate update for the next frame
    fixed_rate_update(start_time, 60, anim_callback);
}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Bouncing Ball Example", 0);

    time_delay_cb(0, anim_callback);

    enable_event_loop();
}
