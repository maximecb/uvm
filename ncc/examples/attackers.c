#include <uvm/syscalls.h>
#include <uvm/window.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define FRAME_WIDTH 800
#define FRAME_HEIGHT 600
#define DOT_SIZE 5

#define BOLT_WIDTH 3
#define BOLT_HEIGHT 12

#define ENEMY_COLS 11
#define ENEMY_ROWS 5

// Ship pixel graphics
char* SHIP_DOTS[1];

// Enemy ship pixel graphics
char* ENEMY_DOTS[4];

// RGBA pixels: 800 * 600
u32 frame_buffer[480_000];

// Left/right arrow currently pressed
bool left_down = false;
bool right_down = false;

// Current enemy i/j (top-left) position
int enemy_i = 0;
int enemy_j = 0;
int enemy_di = 1;

// Bit mask for live enemies
u64 enemies_live = 0x7FFFFFFFFFFFFF;

// Ship position
int ship_x = 400;
int ship_y = 540;

// Ship bolt position
int bolt_x = 0;
int bolt_y = 0;

// Enemy update step counter
unsigned int enemy_steps = 0;

void init()
{
    SHIP_DOTS[0] = (
        "    *    \n"
        "    *    \n"
        " * *** * \n"
        " ******* \n"
        " * *** * \n"
        "*   *   *\n"
    );

    ENEMY_DOTS[0] = (
        "**    **\n"
        "  *  *  \n"
        "  ****  \n"
        " * ** * \n"
        " ****** \n"
        "*      *"
    );

    ENEMY_DOTS[1] = (
        " *    * \n"
        "  *  *  \n"
        "  ****  \n"
        " * ** * \n"
        " ****** \n"
        " *    * "
    );
}

void draw_dots(int xmin, int ymin, int dot_size, char* dots, u32 color)
{
    assert(dots);

    int row = 0;
    int col = 0;

    char* dot = dots;

    for (char* dot = dots; *dot; ++dot)
    {
        char ch = *dot;
        int x = xmin + col * dot_size;
        int y = ymin + row * dot_size;
        col = col + 1;

        if (ch == '\n')
        {
            row = row + 1;
            col = 0;
            continue;
        }

        if (ch != '*')
        {
            continue;
        }

        draw_rect(x, y, dot_size, dot_size, color);
    }
}

int glyph_width(char* dots, int dot_size)
{
    size_t row_len = (size_t)strchr(dots, '\n') - (size_t)dots;
    return dot_size * (int)row_len;
}

int glyph_height(char* dots, int dot_size)
{
    int num_rows = 1;

    for (int i = 0;; ++i)
    {
        char ch = dots[i];

        if (ch == '\n')
            ++num_rows;

        if (ch == '\0')
            break;
    }

    return dot_size * num_rows;
}

void draw_rect(int xmin, int ymin, int width, int height, u32 color)
{
    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < width; ++i)
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

void fire_bolt()
{
    if (bolt_y > 0)
        return;

    int ship_width = glyph_width(SHIP_DOTS[0], DOT_SIZE);

    bolt_x = ship_x + ship_width / 2;
    bolt_y = ship_y - 14;
}

void update_anim()
{
    if (left_down && !right_down)
    {
        if (ship_x > 20)
            ship_x = ship_x - 7;
    }
    else if (right_down && !left_down)
    {
        if (ship_x < FRAME_WIDTH - 60)
            ship_x = ship_x + 7;
    }

    // If our bolt is active, make it move up
    if (bolt_y > 0)
    {
        bolt_y = bolt_y - 14;
    }

    // Clear the screen
    memset(frame_buffer, 0, sizeof(frame_buffer));

    for (int j = 0; j < ENEMY_ROWS; ++j)
    {
        for (int i = 0; i < ENEMY_COLS; ++i)
        {
            u64 enemy_bit = (u64)1 << (ENEMY_COLS * j + i);

            if ((enemies_live & enemy_bit) == 0)
            {
                continue;
            }

            int min_x = 50 + (10 * enemy_i) + (50 * i);
            int min_y = 100 + (10 * enemy_j) + (50 * j);

            if (min_y > 500)
            {
                puts("GAME OVER!\n");
                exit(0);
            }

            char* enemy_glyph = ENEMY_DOTS[enemy_steps % 2];
            int width = glyph_width(enemy_glyph, DOT_SIZE);

            if (bolt_x > min_x && bolt_x < min_x + width)
            {
                int height = glyph_height(enemy_glyph, DOT_SIZE);

                if (bolt_y > min_y && bolt_y < min_y + height)
                {
                    puts("HIT\n");
                    enemies_live = enemies_live ^ enemy_bit;
                    bolt_y = 0;
                }
            }

            draw_dots(
                min_x,
                min_y,
                DOT_SIZE,
                enemy_glyph,
                0xFF_FF_FF
            );
        }
    }

    // Draw our ship
    draw_dots(
        ship_x,
        ship_y,
        DOT_SIZE,
        SHIP_DOTS[0],
        0x00_FF_00
    );

    // Draw our bolt if active
    if (bolt_y > 0)
    {
        draw_rect(
            bolt_x,
            bolt_y,
            BOLT_WIDTH,
            BOLT_HEIGHT,
            0xFF_FF_FF
        );
    }

    window_draw_frame(0, frame_buffer);
}

// Enemy movement update
void update_enemies()
{
    if (enemies_live == 0)
    {
        puts("VICTORY!\n");
        exit(0);
    }

    enemy_i = enemy_i + enemy_di;

    if (enemy_i > 17)
    {
        enemy_di = -1;
        enemy_j = enemy_j + 1;
    }
    else if (enemy_i < 0)
    {
        enemy_di = 1;
        enemy_j = enemy_j + 1;
    }

    // Update the step count
    enemy_steps = enemy_steps + 1;
}

void main()
{
    init();

    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Galactic Attackers", 0);

    Event event;

    for (u64 frame_idx = 0;; frame_idx = frame_idx + 1)
    {
        while (window_poll_event(&event))
        {
            if (event.kind == EVENT_QUIT)
            {
                exit(0);
            }

            if (event.kind == EVENT_KEYDOWN)
            {
                if (event.key == KEY_ESCAPE)
                {
                    exit(0);
                }
                else if (event.key == KEY_LEFT)
                {
                    left_down = true;
                }
                else if (event.key == KEY_RIGHT)
                {
                    right_down = true;
                }
                else if (event.key == KEY_SPACE)
                {
                    fire_bolt();
                }
            }

            if (event.kind == EVENT_KEYUP)
            {
                if (event.key == KEY_LEFT)
                {
                    left_down = false;
                }
                else if (event.key == KEY_RIGHT)
                {
                    right_down = false;
                }
            }
        }

        update_anim();

        // Enemy update rate
        int enemy_rate = 18 - 2 * enemy_j;
        if (enemy_rate <= 2) enemy_rate = 2;

        if (frame_idx > 40 && frame_idx % enemy_rate == 0)
            update_enemies();

        // 60fps max
        thread_sleep(1000 / 60);
    }
}
