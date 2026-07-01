// Signed 8/16-bit division, remainder, and shifts: exercises the
// sign-extend-to-32, operate, then truncate-back-to-width path for sub-word
// signed ops (narrow.c only covers unsigned add/mul).

signed char   s8_div (signed char a, signed char b) { return a / b; }
signed char   s8_rem (signed char a, signed char b) { return a % b; }
signed char   s8_ashr(signed char x, int n)         { return x >> n; }  // arithmetic
short         s16_div(short a, short b)              { return a / b; }
unsigned char u8_shl (unsigned char x, int n)        { return x << n; }  // wraps mod 256

int main()
{
    int r = 0;
    r += s8_div(-100, 3);    // -33
    r += s8_rem(-100, 3);    // -1
    r += s8_ashr(-8, 1);     // -4
    r += s16_div(-1000, 7);  // -142
    r += u8_shl(0x0F, 5);    // 0x1E0 & 0xFF = 0xE0 = 224
    return r + 100;          // 44 + 100 = 144
}
