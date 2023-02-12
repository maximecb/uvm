#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <stdlib.h>

#define FRAME_WIDTH 600
#define FRAME_HEIGHT 600

#define GRID_WIDTH 40
#define GRID_HEIGHT 40
#define TILE_SIZE 15
#define MAX_SNAKE_LEN 2048

// RGBA pixels: 800 * 600
u32 FRAME_BUFFER[480_000];

// Apple position
int apple_x = 10;
int apple_y = 10;

// Current direction
int dx = 0;
int dy = -1;

int snake_len = 5;

// Snake x/y cell positions
int snake_xs[MAX_SNAKE_LEN];
int snake_ys[MAX_SNAKE_LEN];





void draw_square(int xmin, int ymin, int size, u32 color)
{
    for (int j = 0; j < size; ++j)
    {
        for (int i = 0; i < size; ++i)
        {
            u32* pix_ptr = FRAME_BUFFER + (FRAME_WIDTH) * (ymin + j) + (xmin + i);
            *pix_ptr = color;
        }
    }
}

void draw_circle(int xmin, int ymin, int size, u32 color)
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
            int dx = x - cx;
            int dy = y - cy;
            int dist_sqr = (dx * dx) + (dy * dy);

            if (dist_sqr > r2)
                continue;

            u32* pix_ptr = FRAME_BUFFER + (FRAME_WIDTH * y + x);
            *pix_ptr = color;
        }
    }
}

void anim_callback()
{
    // Clear the screen
    memset(FRAME_BUFFER, 0, 1_920_000);

    for (int i = 0; i < GRID_WIDTH; ++i)
    {
        draw_square(i * TILE_SIZE, 0, TILE_SIZE, 0x873E23);
        draw_square(i * TILE_SIZE, (GRID_HEIGHT-1) * TILE_SIZE, TILE_SIZE, 0x873E23);
    }

    for (int j = 0; j < GRID_HEIGHT; ++j)
    {
        draw_square(0, j * TILE_SIZE, TILE_SIZE, 0x873E23);
        draw_square((GRID_WIDTH-1) * TILE_SIZE, j * TILE_SIZE, TILE_SIZE, 0x873E23);
    }

    // Draw the apple
    draw_circle(apple_x * GRID_WIDTH, apple_y * GRID_HEIGHT, TILE_SIZE, 0xFF0000);










    window_draw_frame(0, FRAME_BUFFER);
    time_delay_cb(200, anim_callback);
}

void keydown(u64 window_id, u16 keycode)
{
    print_str("keydown: ");
    print_i64(keycode);
    print_endl();






}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Snake Game Example", 0);
    window_on_keydown(0, keydown);

    time_delay_cb(0, anim_callback);
    enable_event_loop();
}
