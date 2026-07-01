// Integer arithmetic across widths, plus sign/zero extension and truncation.

int arith(int a, int b)
{
    int sum  = a + b;
    int diff = a - b;
    int prod = a * b;
    int quot = a / b;
    int rem  = a % b;
    int neg  = -a;
    return sum + diff + prod + quot + rem + neg;
}

unsigned bitops(unsigned a, unsigned b)
{
    unsigned r = a & b;
    r |= a ^ b;
    r <<= 1;          // shl
    r >>= 2;          // lshr (unsigned)
    int sr = (int)r >> 3;  // ashr (signed)
    return r | (a & 0xFF) | (unsigned)sr;
}

long widen(int x)
{
    long s = x;                 // sext i32 -> i64
    short t = (short)x;         // trunc i32 -> i16
    unsigned long u = (unsigned)x;  // zext i32 -> i64
    return s + t + (long)u;
}

int main()
{
    return arith(20, 6) + (int)bitops(0xF0, 0x0F) + (int)widen(-3);
}
