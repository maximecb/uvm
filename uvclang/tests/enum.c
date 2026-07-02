// Enums -- untested until now, though DOOM declares 42 of them. Enum constants
// lower to plain ints; this pins down that the front-end assigns each the right
// value across the initializer forms DOOM actually uses:
//   * implicit auto-increment;
//   * explicit integer values;
//   * char-literal initializers (DOOM_KEY_A = 'a');
//   * constant-expression initializers (DOOM_KEY_CTRL = 0x80 + 0x1d);
//   * negative values (DOOM_KEY_UNKNOWN = -1);
//   * auto-increment resuming after an explicit value;
// plus use as an array size, an array index, and switch labels.

enum color { RED, GREEN, BLUE };                 // 0, 1, 2

enum key {
    K_UNKNOWN = -1,                              // negative
    K_TAB     = 9,
    K_ENTER   = 13,
    K_A       = 'a',                             // 97
    K_B,                                         // 98 (auto after 'a')
    K_C,                                         // 99
    K_CTRL    = (0x80 + 0x1d),                   // 157
    K_SHIFT   = (0x80 + 0x36)                    // 182
};

typedef enum { MON, TUE, WED, THU, FRI } day_t;  // typedef enum

static int score[BLUE + 1] = { 10, 20, 30 };     // enum constant as array size

static int classify(enum key k)
{
    switch (k) {                                 // enum value in a switch
    case K_UNKNOWN:              return 1;
    case K_A: case K_B: case K_C: return 2;
    case K_CTRL:                 return 3;
    default:                     return 0;
    }
}

int main(void)
{
    int total = 0;

    total += RED + GREEN + BLUE;                          // 0+1+2 = 3
    total += score[RED] + score[GREEN] + score[BLUE];     // 10+20+30 = 60
    total += K_A + K_B + K_C;                             // 97+98+99 = 294
    total += K_CTRL + K_SHIFT;                            // 157+182 = 339
    total += K_UNKNOWN;                                   // -1
    total += classify(K_A) + classify(K_CTRL) + classify(K_UNKNOWN); // 2+3+1 = 6

    day_t d = WED;                                        // 2
    total += d;

    return total % 256;   // 3+60+294+339-1+6+2 = 703 -> 703 % 256 = 191
}
