// 8- and 16-bit arithmetic with well-defined unsigned wrap-around, plus the
// sign/zero-extension corner cases around truncation and narrow comparisons.

unsigned char  u8_add(unsigned char a, unsigned char b)   { return a + b; } // mod 256
unsigned short u16_mul(unsigned short a, unsigned short b) { return a * b; } // mod 65536
signed char    s8_narrow(int x)         { return (signed char)x; }          // trunc i32 -> i8
int            s8_to_int(signed char c) { return c; }                       // sext i8 -> i32
unsigned       u8_to_uint(unsigned char c) { return c; }                    // zext i8 -> i32
int            cmp_s8(signed char a, signed char b)     { return a < b; }   // signed i8 compare
int            cmp_u8(unsigned char a, unsigned char b) { return a < b; }   // unsigned i8 compare

int main()
{
    int r = 0;
    r += u8_add(200, 100);               // 44
    r += u16_mul(300, 300);              // 24464
    r += s8_to_int(s8_narrow(0x1FF));    // (signed char)0xFF == -1
    r += (int)u8_to_uint(0xAB);          // 171
    r += cmp_s8(-1, 1);                  // 1  (-1 < 1)
    r += cmp_u8(255, 1);                 // 0  (255 < 1 is false)
    return r & 0xFFFF;
}
