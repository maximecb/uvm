#include <uvm/syscalls.h>
#include <uvm/utils.h>
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
u32 FRAME_BUFFER[480_000];

// Left/right arrow currently pressed
bool left_down = false;
bool right_down = false;

// Current enemy i/j (top-left) position
int enemy_i = 0;
int enemy_j = 0;
int enemy_di = 1;

// Bit mask for live enemies
u64 enemies_live = 0xFFFFFFFFFFFFFF;

// Ship position
int ship_x = 400;
int ship_y = 540;

// Ship bolt position
int bolt_x = 0;
int bolt_y = 0;

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

void fire_bolt()
{
    if (bolt_y > 0)
        return;

    int ship_width = glyph_width(SHIP_DOTS[0], DOT_SIZE);

    bolt_x = ship_x + ship_width / 2;
    bolt_y = ship_y - 14;
}

void anim_callback()
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

    if (bolt_y > 0)
    {
        bolt_y = bolt_y - 14;
    }

    // Clear the screen
    memset(FRAME_BUFFER, 0, 1_920_000);

    for (int j = 0; j < ENEMY_ROWS; ++j)
    {
        for (int i = 0; i < ENEMY_COLS; ++i)
        {
            int enemy_bit = 1 << (ENEMY_COLS * j + i);

            if ((enemies_live & enemy_bit) == 0)
            {
                continue;
            }

            int min_x = 50 + (10 * enemy_i) + (50 * i);
            int min_y = 100 + (10 * enemy_j) + (50 * j);

            int width = glyph_width(ENEMY_DOTS[0], DOT_SIZE);

            if (bolt_x > min_x && bolt_x < min_x + width)
            {
                int height = glyph_height(ENEMY_DOTS[0], DOT_SIZE);

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
                ENEMY_DOTS[0],
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

    window_draw_frame(0, FRAME_BUFFER);
    time_delay_cb(20, anim_callback);
}

// Enemy movement update
void enemy_callback()
{
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

    print_i64(enemy_i);
    print_endl();

    time_delay_cb(750, enemy_callback);
}

void keydown(u64 window_id, u16 keycode)
{
    if (keycode == KEY_LEFT)
    {
        left_down = true;
    }
    else if (keycode == KEY_RIGHT)
    {
        right_down = true;
    }
    else if (keycode == KEY_SPACE)
    {
        fire_bolt();
    }
}

void keyup(u64 window_id, u16 keycode)
{
    if (keycode == KEY_LEFT)
    {
        left_down = false;
    }
    else if (keycode == KEY_RIGHT)
    {
        right_down = false;
    }
}

void main()
{
    init();

    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Galactic Attackers", 0);
    window_on_keydown(0, keydown);
    window_on_keyup(0, keyup);

    time_delay_cb(0, anim_callback);
    time_delay_cb(1500, enemy_callback);

    enable_event_loop();
}
