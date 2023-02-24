#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <stdlib.h>
#include <stdint.h>

#define FRAME_WIDTH 600
#define FRAME_HEIGHT 600
#define CELL_SIZE 4
#define NUM_ROWS 150
#define NUM_COLS 150

// RGBA pixels: 600 * 600
uint32_t frame_buffer[360_000];

// Current and next board
bool board[2][NUM_ROWS][NUM_COLS];

// Current generation counter
uint32_t gen_no = 0;

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

int get_cell(uint32_t board_idx, int row, int col)
{
    return (int)board[board_idx][(uint32_t)row % NUM_ROWS][(uint32_t)col % NUM_COLS];
}

int count_neighbors(uint32_t board_idx, int row, int col)
{
    int count = 0;

    // Row above
    count = count + get_cell(board_idx, row - 1, col - 1);
    count = count + get_cell(board_idx, row - 1, col    );
    count = count + get_cell(board_idx, row - 1, col + 1);

    // Middle row
    count = count + get_cell(board_idx, row, col - 1);
    count = count + get_cell(board_idx, row, col + 1);

    // Row below
    count = count + get_cell(board_idx, row + 1, col - 1);
    count = count + get_cell(board_idx, row + 1, col    );
    count = count + get_cell(board_idx, row + 1, col + 1);

    return count;
}

void update()
{
    // Clear the screen
    memset(frame_buffer, 0, 1_440_000);

    uint32_t prev_board = gen_no % 2;
    uint32_t next_board = (gen_no + 1) % 2;
    gen_no = gen_no + 1;

    for (int row = 0; row < NUM_ROWS; ++row)
    {
        for (int col = 0; col < NUM_COLS; ++col)
        {
            int count = count_neighbors(prev_board, row, col);
            int alive = get_cell(prev_board, row, col);

            if (alive)
            {
                if (count < 2)
                    alive = 0;
                else if (count > 3)
                    alive = 0;
            }
            else
            {
                if (count == 3)
                    alive = 1;
            }

            board[next_board][row][col] = alive;

            if (alive)
            {
                draw_rect(
                    col * CELL_SIZE,
                    row * CELL_SIZE,
                    CELL_SIZE,
                    CELL_SIZE,
                    0x00_EE_00
                );
            }
        }
    }

    window_draw_frame(0, frame_buffer);
}

void anim_callback()
{
    benchmark(update());

    time_delay_cb(250, anim_callback);
    //time_delay_cb(0, anim_callback);
}

void main()
{
    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Game of Life", 0);

    // Randomly initialize the board
    srand(time_current_ms());
    for (int row = 0; row < NUM_ROWS; ++row)
    {
        for (int col = 0; col < NUM_COLS; ++col)
        {
            board[0][row][col] = rand() % 2;
        }
    }

    time_delay_cb(0, anim_callback);
    enable_event_loop();
}
