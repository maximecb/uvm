// Integer width conversions: sign extension, zero extension, truncation,
// and signed/unsigned 64-bit arithmetic.

int sext_trunc(int x)
{
    signed char c = (signed char)x;  // trunc i32 -> i8
    short s = (short)x;              // trunc i32 -> i16
    long l = c;                       // sext i8 -> i64
    return (int)(l + s);
}

unsigned zext(unsigned char b, unsigned short h)
{
    unsigned long lb = b;   // zext i8  -> i64
    unsigned long lh = h;   // zext i16 -> i64
    return (unsigned)(lb + lh);
}

long mul64(int a, int b)
{
    return (long)a * (long)b;   // sext i32 -> i64, then 64-bit multiply
}

int main()
{
    int a = sext_trunc(-1);            // -2
    unsigned b = zext(200, 300);       // 500
    long c = mul64(1000, 1000) % 97;   // 27
    return (a & 0xFF) + (int)b + (int)c;
}
