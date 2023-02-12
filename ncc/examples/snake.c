#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <stdlib.h>
#include <stdio.h>

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
int dy = 1;

int snake_len = 5;

// Snake x/y cell positions, from tail to head
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

bool snake_collision(int x, int y)
{
    for (int i = 0; i < snake_len; ++i)
    {
        if (snake_xs[i] == x && snake_ys[i] == y)
        {
            return true;
        }
    }

    return false;
}

void spawn_apple()
{
    while (true)
    {
        int nx = 2 + rand() % (GRID_WIDTH - 3);
        int ny = 2 + rand() % (GRID_HEIGHT - 3);

        if (snake_collision(nx, ny) == false)
        {
            apple_x = nx;
            apple_y = ny;
            break;
        }
    }
}

void anim_callback()
{
    // Move the snake body forward
    for (int i = 0; i < snake_len - 1; ++i)
    {
        snake_xs[i] = snake_xs[i+1];
        snake_ys[i] = snake_ys[i+1];
    }

    int nx = snake_xs[snake_len - 1] + dx;
    int ny = snake_ys[snake_len - 1] + dy;

    if (snake_collision(nx, ny))
    {
        puts("snake ran into itself\n");
        exit(0);
    }

    if (nx == 0 || nx >= GRID_WIDTH - 1 || ny == 0 || ny == GRID_HEIGHT - 1)
    {
        puts("snake crashed into wall\n");
        exit(0);
    }

    if (nx == apple_x && ny == apple_y)
    {
        puts("got the apple\n");

        snake_xs[snake_len] = apple_x;
        snake_ys[snake_len] = apple_y;
        ++snake_len;

        spawn_apple();
    }

    // Move the head forward
    snake_xs[snake_len - 1] = nx;
    snake_ys[snake_len - 1] = ny;











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
    draw_circle(apple_x * TILE_SIZE, apple_y * TILE_SIZE, TILE_SIZE, 0xFF0000);

    // Draw the snake
    for (int i = 0; i < snake_len - 1; ++i)
    {
        draw_square(
            snake_xs[i] * TILE_SIZE,
            snake_ys[i] * TILE_SIZE,
            TILE_SIZE,
            0x00FF00
        );
    }

    // Draw the head
    draw_square(
        snake_xs[snake_len - 1] * TILE_SIZE,
        snake_ys[snake_len - 1] * TILE_SIZE,
        TILE_SIZE,
        0xFF00FF
    );








    window_draw_frame(0, FRAME_BUFFER);
    time_delay_cb(100, anim_callback);
}

void keydown(u64 window_id, u16 keycode)
{
    int sdx = snake_xs[snake_len - 2] - snake_xs[snake_len - 1];
    int sdy = snake_ys[snake_len - 2] - snake_ys[snake_len - 1];

    if (keycode == KEY_LEFT && sdx != -1)
    {
        dx = -1;
        dy = 0;
    }
    else if (keycode == KEY_RIGHT && sdx != 1)
    {
        dx = 1;
        dy = 0;
    }
    else if (keycode == KEY_UP && sdy != -1)
    {
        dx = 0;
        dy = -1;
    }
    else if (keycode == KEY_DOWN && sdy != 1)
    {
        dx = 0;
        dy = 1;
    }
}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Snake Game Example", 0);
    window_on_keydown(0, keydown);

    for (int i = 0; i < snake_len; ++i)
    {
        snake_xs[i] = GRID_WIDTH / 2;
        snake_ys[i] = ((GRID_HEIGHT / 4) - snake_len) + i;
    }

    time_delay_cb(0, anim_callback);
    enable_event_loop();
}
