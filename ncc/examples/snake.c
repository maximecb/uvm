#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <uvm/window.h>
#include <stdlib.h>
#include <stdio.h>

#define FRAME_WIDTH 640
#define FRAME_HEIGHT 640

#define GRID_WIDTH 32
#define GRID_HEIGHT 32
#define TILE_SIZE 20
#define MAX_SNAKE_LEN 2048

// RGBA pixels: 640 * 640
u32 frame_buffer[409_600];

// Apple position
int apple_x = 10;
int apple_y = 10;

// Current direction
int dx = 0;
int dy = 1;

int snake_len = 5;

// Snake x/y cell positions, from head to tail
int snake_xs[MAX_SNAKE_LEN];
int snake_ys[MAX_SNAKE_LEN];

void draw_square(int xmin, int ymin, int size, u32 color)
{
    for (int j = 0; j < size; ++j)
    {
        for (int i = 0; i < size; ++i)
        {
            u32* pix_ptr = frame_buffer + (FRAME_WIDTH) * (ymin + j) + (xmin + i);
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

            u32* pix_ptr = frame_buffer + (FRAME_WIDTH * y + x);
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
        int nx = 2 + rand() % (GRID_WIDTH - 4);
        int ny = 2 + rand() % (GRID_HEIGHT - 4);

        if (snake_collision(nx, ny) == false)
        {
            apple_x = nx;
            apple_y = ny;
            break;
        }
    }
}

void update()
{
    // Move the snake body forward
    // We do this from tail to head
    for (int i = snake_len - 1; i > 0; i = i - 1)
    {
        snake_xs[i] = snake_xs[i-1];
        snake_ys[i] = snake_ys[i-1];
    }

    int nx = snake_xs[0] + dx;
    int ny = snake_ys[0] + dy;

    if (snake_collision(nx, ny))
    {
        puts("snake ran into itself\n");
        exit(0);
    }

    // Move the head forward (after the collision check)
    snake_xs[0] = nx;
    snake_ys[0] = ny;

    if (nx == 0 || nx >= GRID_WIDTH - 1 || ny == 0 || ny == GRID_HEIGHT - 1)
    {
        puts("snake crashed into wall\n");
        exit(0);
    }

    if (nx == apple_x && ny == apple_y)
    {
        spawn_apple();
        ++snake_len;

        puts("got the apple, snake length is now ");
        print_i64(snake_len);
        puts("!\n");
    }

    // Clear the screen
    memset(frame_buffer, 0, sizeof(frame_buffer));

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
    for (int i = 1; i < snake_len - 1; ++i)
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
        snake_xs[0] * TILE_SIZE,
        snake_ys[0] * TILE_SIZE,
        TILE_SIZE,
        0xFF00FF
    );

    window_draw_frame(0, frame_buffer);

    thread_sleep(100);
}

void read_keys()
{
    Event event;

    while (window_poll_event(&event))
    {
        if (event.kind == EVENT_QUIT)
        {
            exit(0);
        }

        if (event.kind != EVENT_KEYDOWN)
        {
            continue;
        }

        // Current snake x/y direction
        int sdx = snake_xs[1] - snake_xs[0];
        int sdy = snake_ys[1] - snake_ys[0];

        if (event.key == KEY_ESCAPE)
        {
            exit(0);
        }

        if (event.key == KEY_LEFT && sdx != -1)
        {
            dx = -1;
            dy = 0;
        }
        else if (event.key == KEY_RIGHT && sdx != 1)
        {
            dx = 1;
            dy = 0;
        }
        else if (event.key == KEY_UP && sdy != -1)
        {
            dx = 0;
            dy = -1;
        }
        else if (event.key == KEY_DOWN && sdy != 1)
        {
            dx = 0;
            dy = 1;
        }
    }
}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Snake Game Example", 0);

    // Initialize the snake positions
    for (int i = 0; i < snake_len; ++i)
    {
        snake_xs[i] = GRID_WIDTH / 2;
        snake_ys[i] = (GRID_HEIGHT / 4) - i;
    }

    for (;;)
    {
        read_keys();
        update();
    }
}
