//
// Toledo Nanochess example for UVM
//
// by Oscar Toledo G. (nanochess)
// https://nanochess.org/
// https://twitter.com/nanochess
//

#include <uvm/syscalls.h>
#include <uvm/utils.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define FRAME_WIDTH 800
#define FRAME_HEIGHT 600
#define DOT_SIZE 8

// Chess pieces graphics
char* PAWN_DOTS[1];
char* KNIGHT_DOTS[1];
char* BISHOP_DOTS[1];
char* ROOK_DOTS[1];
char* QUEEN_DOTS[1];
char* KING_DOTS[1];
char* CURSOR_DOTS[1];

// RGBA pixels: 800 * 600
u32 frame_buffer[480_000];

// Left/right arrow currently pressed
bool left_key = false;
bool right_key = false;
bool up_key = false;
bool down_key = false;
bool space_key = false;

int cursor = 64;
int selected;

int B;
int i;
int y;
int u;
int b;
int I[781104];
int x=10;
int z=15;
int M=10000;
char *pieces = "ustvrtsuqqqqqqqqyyyyyyyy}{|~z|{}";
char *l = "   76Lsabcddcba .pknbrq  PKNBRQ ?A6J57IKJT576,+-48HLSU";

void init()
{
    PAWN_DOTS[0] = (
        "        \n"
        "        \n"
        "   **   \n"
        "  ****  \n"
        "  ****  \n"
        "   **   \n"
        "  ****  \n"
        "        \n"
    );

    KNIGHT_DOTS[0] = (
        "        \n"
        " ***    \n"
        " * **   \n"
        " *****  \n"
        " ** *** \n"
        "   **** \n"
        "  ***** \n"
        "        \n"
    );

    BISHOP_DOTS[0] = (
        "        \n"
        "   **   \n"
        "  ****  \n"
        "  ****  \n"
        "  ****  \n"
        "   **   \n"
        " **  ** \n"
        "        \n"
    );

    ROOK_DOTS[0] = (
        "        \n"
        " * ** * \n"
        " ****** \n"
        "  ****  \n"
        "  ****  \n"
        " ****** \n"
        " ****** \n"
        "        \n"
    );

    QUEEN_DOTS[0] = (
        "        \n"
        " * ** * \n"
        " * ** * \n"
        "  *  *  \n"
        "  ****  \n"
        "  ****  \n"
        "  ****  \n"
        "        \n"
    );

    KING_DOTS[0] = (
        "        \n"
        "  ****  \n"
        " ** * * \n"
        " * * ** \n"
        "  ****  \n"
        "  ****  \n"
        "  ****  \n"
        "        \n"
    );

    CURSOR_DOTS[0] = (
        "********\n"
        "*      *\n"
        "*      *\n"
        "*      *\n"
        "*      *\n"
        "*      *\n"
        "*      *\n"
        "********\n"
    );

    // Setup the chessboard.
    for (int c = 0; c < 120; ++c) {
        I[c] = 7;
    }
    int e = 0;
    for (int c = 21; c < 101; c = c + 10) {
        for (int d = 0; d < 8; ++d) {
            if (c >= 41 && c <= 71) {
                I[c + d] = 0;
            } else {
                I[c + d] = pieces[e] & 31;
                ++e;
            }
        }
    }
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

// Adapted to ncc from https://nanochess.org/chess3.html
int X(int w, int c, int h, int e, int S, int s)
{
    int t;
    int o;
    int L;
    int E;
    int d;
    int O=21;
    int N=-M*M;
    int K=78-h<<x;
    int p;
    int *g;
    int n;
    int *m;
    int A;
    int q;
    int r;
    int C;
    int J;
    int a;

    if (y)
        a = -x;
    else
        a = x;
    y = y ^ 8;
    d = 0;
    if (w != 0)
        d = w;
    else {
        d = 0;
        if (s != 0 && s >= h) {
            if (X(0,0,0,21, 0,0)>M)
                d = 1;
        }
    }
    do {
        p = O;
        o = I[p];
        if (o != 0) {
            q = o & z ^ y;
            if (q < 7) {
                // A=q&2?8:4; unsupported
                if (q & 2)
                    A = 8;
                else
                    A = 4;
                --q;
                if (o - 9 & z) {
                    if (q == 0)
                        C = 38;
                    else if (q == 2)
                        C = 46;
                    else if (q == 3)
                        C = 36;
                    else
                        C = 32;
                } else
                    C = 42;
                while (1) {
                    p = p + l[C] - 64;
                    r = I[p];
                    if (!w || p == w) {
                        if (q | p + a - S)
                            g = 0;
                        else
                            g = I + S;
                        if (!r & (q | A < 3 || g) || (r + 1 & z ^ y) > 9 && q | A > 2){
                            m=!(r-2&7);
                            if (m) {
                                y = y ^ 8;
                                return K;
                            }
                            n = o & z;
                            J = n;
                            E = I[p - a] & z;
                            if (q | E - 7)
                                t = n;
                            else {
                                n = n + 2;
                                t = 6 ^ y;
                            }

                            while( n<=t){
                                if (r)
                                    L = l[r&7]*9-189-h-q ;
                                else
                                    L = 0;
                                if (s) {
                                    if (1 - q)
                                        L = L + l[p/x+5]-l[O/x+5]+l[p%x+6]*-~!q-l[O%x+6]+o/16*8;
                                    else if (m)
                                        L = L + 9;
                                    if (!q) {
                                        if (g)
                                            L = L + 99;
                                        L = L + !(I[p-1]^n)+!(I[p+1]^n)+l[n&7]*9-386+(A<2);
                                    }
                                    L = L + !(E^y^9);
                                }
//                                puts("Moving ");
//                                print_i64(O);
//                                puts(" to ");
//                                print_i64(p);
//                                puts("\n");
                                if (s > h || 1 < s && s == h && L > z || d) {
                                    I[p] = n;
                                    if (m) {
                                        *g = *m;
                                        *m = 0;
                                    } else if (g) {
                                        *g = 0;
                                    }
                                    I[O] = 0;
                                    if (q | A > 1)
                                        J = 0;
                                    else
                                        J = p;
//                                    puts("Calling ");
//                                    print_i64(h + 1);
//                                    puts("\n");
                                    if (s > h || d) {
                                        L = L - X(0, L - N, h + 1, 21, J, s);
                                    } else {
                                        L = L - X(p, L - N, h + 1, 21, J, s);
                                    }
//                                    puts("Returning from ");
//                                    print_i64(h + 1);
//                                    puts("\n");
//                                    L = L - X(s>h|d?0:p,L-N,h+1,21,J,s); doesn't work
                                    if(!(h||s-1|B-O|i-n|p-b|L<-M)) {
                                        u = J;
                                        return u;
                                    }
                                    J=q-1|A<7||m||!s|d|r|o<z||X(0,0,0,21, 0,0)>M;
                                    I[O]=o;
                                    I[p]=r;
                                    if (m) {
                                        *m = *g;
                                        *g = 0;
                                    } else if (g) {
                                        *g = 9 ^ y;
                                    }
                                }
                                if( L>N){
                                    if( s>1){
                                        if( h&&c-L<0) {
                                            y = y ^ 8;
                                            return L;
                                        }
                                        if(!h) {
                                            i=n;
                                            B=O;
                                            b=p;
                                        }
                                    }
                                    N=L;
                                }
                                if (J) {
                                    n = n + J;
                                } else {
                                    g = I + p;
                                    if (p < O) {
                                        // m = g - 3; Doubt
                                        // m[O - p]; Doubt
                                        // p = p + (p - O); Doubt
                                        m = g;
                                        --m;
                                        if (*m) {
                                            ++n;
                                        } else {
                                            --m;
                                            if (*m) {
                                                ++n;
                                            } else {
                                                --m;
                                                if (*m < z) {
                                                    ++n;
                                                } else {
                                                    --p;
                                                    if (I[p])
                                                        ++n;
                                                }
                                            }
                                        }
                                    } else {
                                        m = g;
                                        ++m;
                                        if (*m) {
                                            ++n;
                                        } else {
                                            ++m;
                                            if (*m < z) {
                                                ++n;
                                            } else {
                                                ++p;
                                                if (I[p])
                                                    ++n;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (!r && q > 2)
                        continue;
                    p = O;
                    if (q|A>2|o>z&!r&&++C*--A)
                        continue;
                    break;
                }
            }
        }
        ++O;
    } while (O < 99) ;
    y = y ^ 8;
    if (N+M*M && N > -K + 1924 | d)
        return N;
    return 0;
}

#define SQUARE_WHITE    0xffc0a080
#define SQUARE_BLACK    0xff8080c0
#define SQUARE_SEL      0xffff8040

#define PIECE_WHITE    0xffffffff
#define PIECE_BLACK    0xff000000

#define CURSOR_COLOR   0xffffe0c0

void computer_callback()
{
    X(0,0,0,21,u,3);
    X(0,0,0,21,u,1);
}

void anim_callback()
{
    if (left_key) {
        left_key = false;
        if (cursor % 10 == 1)
            cursor = cursor + 7;
        else
            cursor = cursor - 1;
    }
    if (right_key) {
        right_key = false;
        if (cursor % 10 == 8)
            cursor = cursor - 7;
        else
            cursor = cursor + 1;
    }
    if (up_key) {
        up_key = false;
        if (cursor < 30)
            cursor = cursor + 70;
        else
            cursor = cursor - 10;
    }
    if (down_key) {
        down_key = false;
        if (cursor > 90)
            cursor = cursor - 70;
        else
            cursor = cursor + 10;
    }
    if (space_key) {
        space_key = false;
        if (I[cursor] != 0 && ((I[cursor] ^ y) & 8) == 8) {
            selected = cursor;
        } else if (selected != 0) {
            B = selected;
            b = cursor;
            i = I[B] & 0x0f;
            if (b < 30 && (i & 7) == 1)     // Promotion fixed to queen
                i = i ^ 7;
            int prev_turn = y;
            X(0,0,0,21,u,1);
            selected = 0;
            if (y != prev_turn) {
                time_delay_cb(100, computer_callback);
            }
        }
    }

    // Clear the screen
    memset(frame_buffer, 0, sizeof(frame_buffer));

    for (int j = 0; j < 8; ++j)
    {
        for (int i = 0; i < 8; ++i)
        {
            int min_x = 144 + i * 64;
            int min_y = 44 + j * 64;
            int color;

            if (((j ^ i) & 1) != 0)
                color = SQUARE_BLACK;
            else
                color = SQUARE_WHITE;
            if (selected == j * 10 + 21 + i)
                color = SQUARE_SEL;
            draw_rect(
                min_x,
                min_y,
                64,
                64,
                color
            );

            if (cursor == j * 10 + 21 + i)
                draw_dots(
                          min_x,
                          min_y,
                          DOT_SIZE,
                          CURSOR_DOTS[0],
                          CURSOR_COLOR
                          );

            int c = I[j * 10 + 21 + i];

            if ((c & 7) == 1)
                draw_dots(
                    min_x,
                    min_y,
                    DOT_SIZE,
                    PAWN_DOTS[0],
                    (c & 8) ? PIECE_WHITE : PIECE_BLACK
                );
            else if ((c & 7) == 3)
                draw_dots(
                    min_x,
                    min_y,
                    DOT_SIZE,
                    KNIGHT_DOTS[0],
                    (c & 8) ? PIECE_WHITE : PIECE_BLACK
                );
            else if ((c & 7) == 4)
                draw_dots(
                    min_x,
                    min_y,
                    DOT_SIZE,
                    BISHOP_DOTS[0],
                    (c & 8) ? PIECE_WHITE : PIECE_BLACK
                );
            else if ((c & 7) == 5)
                draw_dots(
                    min_x,
                    min_y,
                    DOT_SIZE,
                    ROOK_DOTS[0],
                    (c & 8) ? PIECE_WHITE : PIECE_BLACK
                );
            else if ((c & 7) == 6)
                draw_dots(
                    min_x,
                    min_y,
                    DOT_SIZE,
                    QUEEN_DOTS[0],
                    (c & 8) ? PIECE_WHITE : PIECE_BLACK
                );
            else if ((c & 7) == 2)
                draw_dots(
                    min_x,
                    min_y,
                    DOT_SIZE,
                    KING_DOTS[0],
                    (c & 8) ? PIECE_WHITE : PIECE_BLACK
                );
        }
    }

    window_draw_frame(0, frame_buffer);
    time_delay_cb(33, anim_callback);
}

void keydown(u64 window_id, u16 keycode)
{
    if (keycode == KEY_LEFT)
    {
        left_key = true;
    }
    else if (keycode == KEY_RIGHT)
    {
        right_key = true;
    }
    else if (keycode == KEY_UP)
    {
        up_key = true;
    }
    else if (keycode == KEY_DOWN)
    {
        down_key = true;
    }
    else if (keycode == KEY_SPACE)
    {
        space_key = true;
    }
}

void keyup(u64 window_id, u16 keycode)
{
    if (keycode == KEY_LEFT)
    {
        left_key = false;
    }
    else if (keycode == KEY_RIGHT)
    {
        right_key = false;
    }
    else if (keycode == KEY_UP)
    {
        up_key = false;
    }
    else if (keycode == KEY_DOWN)
    {
        down_key = false;
    }
    else if (keycode == KEY_SPACE)
    {
        space_key = false;
    }
}

void main()
{
    init();

    window_create(FRAME_WIDTH, FRAME_HEIGHT, "Toledo Nanochess for UVM", 0);
    window_on_keydown(0, keydown);
    window_on_keyup(0, keyup);

    time_delay_cb(0, anim_callback);

    enable_event_loop();
}
