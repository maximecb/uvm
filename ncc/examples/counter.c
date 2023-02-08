#include <assert.h>
#include <uvm/syscalls.h>
#include <uvm/utils.h>

size_t FRAME_WIDTH = 800;
size_t FRAME_HEIGHT = 600;
u32 CH_DOTS_X = 5;
u32 CH_DOTS_Y = 7;

// RGBA pixels: 800 * 600
u32 FRAME_BUFFER[480_000];

// Strings mapping the dots for each character
char* CHAR_DOTS[256] = 0;

u64 start_time = 0;

void init_dots()
{
    CHAR_DOTS['0'] = (
        " *** "
        "*   *"
        "*  **"
        "* * *"
        "**  *"
        "*   *"
        " *** "
    );

    CHAR_DOTS['1'] = (
        "  *  "
        " **  "
        "  *  "
        "  *  "
        "  *  "
        "  *  "
        " *** "
    );

    CHAR_DOTS['2'] = (
        " *** "
        "*   *"
        "   * "
        "  *  "
        " *   "
        "*    "
        "*****"
    );

    CHAR_DOTS['3'] = (
        " *** "
        "*   *"
        "    *"
        "  ***"
        "    *"
        "*   *"
        " *** "
    );

    CHAR_DOTS['4'] = (
        "*   *"
        "*   *"
        "*   *"
        "*****"
        "    *"
        "    *"
        "    *"
    );

    CHAR_DOTS['5'] = (
        "*****"
        "*    "
        "*    "
        "**** "
        "    *"
        "    *"
        "**** "
    );

    CHAR_DOTS['6'] = (
        " *** "
        "*   *"
        "*    "
        "**** "
        "*   *"
        "*   *"
        " *** "
    );

    CHAR_DOTS['7'] = (
        "*****"
        "    *"
        "   * "
        "  *  "
        "  *  "
        "  *  "
        "  *  "
    );

    CHAR_DOTS['8'] = (
        "*****"
        "*   *"
        "*   *"
        "*****"
        "*   *"
        "*   *"
        "*****"
    );

    CHAR_DOTS['9'] = (
        "*****"
        "*   *"
        "*   *"
        "*****"
        "    *"
        "    *"
        "*****"
    );

    CHAR_DOTS['F'] = (
        "*****"
        "*    "
        "*    "
        "*****"
        "*    "
        "*    "
        "*    "
    );

    CHAR_DOTS['P'] = (
        "**** "
        "*   *"
        "*   *"
        "**** "
        "*    "
        "*    "
        "*    "
    );

    CHAR_DOTS['S'] = (
        " *** "
        "*   *"
        "*    "
        "**** "
        "    *"
        "*   *"
        " *** "
    );
}

void draw_circle(int xmin, int ymin, int size)
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
            *pix_ptr = 0xFF_00_00;
        }
    }
}

void draw_char(int xmin, int ymin, int dot_size, char ch)
{
    char* dots = CHAR_DOTS[ch];
    assert(dots);

    for (int j = 0; j < CH_DOTS_Y; ++j)
    {
        for (int i = 0; i < CH_DOTS_X; ++i)
        {
            bool dot_active = dots[j * CH_DOTS_X + i] == '*';

            if (!dot_active)
                continue;

            int x = xmin + i * dot_size;
            int y = ymin + j * dot_size;
            draw_circle(x, y, dot_size);
        }
    }
}

void draw_number(int xmax, int ymin, int dot_size, int number)
{
    int num_digits = 0;
    for (int n = number; n > 0; n = n / 10)
    {
        ++num_digits;
    }

    for (int i = 0; i < num_digits; ++i)
    {
        int digit = number % 10;
        number = number / 10;

        draw_char(
            xmax - 5 * dot_size * i,
            ymin,
            dot_size,
            '0' + digit
        );
    }
}

void anim_callback()
{
    // Clear the screen
    memset(FRAME_BUFFER, 0, 1_920_000);

    u64 delta_time = time_current_ms() - start_time;
    u64 seconds = delta_time / 10;
    int s = asm (seconds) -> int {};

    draw_number(500, 200, 10, s);

    window_draw_frame(0, FRAME_BUFFER);
    time_delay_cb(25, anim_callback);
}

void main()
{
    init_dots();

    start_time = time_current_ms();

    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Counter", 0);
    window_show(0);

    time_delay_cb(0, anim_callback);
    enable_event_loop();
}
