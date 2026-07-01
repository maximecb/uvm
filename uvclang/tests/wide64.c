// 64-bit edge values: LLONG_MIN, wide shifts, unsigned high-bit division, and
// signed-vs-unsigned 64-bit comparison divergence. Well-defined throughout
// (no LLONG_MIN/-1, no negation of LLONG_MIN).

#include <limits.h>

long long          sdiv64(long long a, long long b)                   { return a / b; }
unsigned long long udiv64(unsigned long long a, unsigned long long b) { return a / b; }
long long          ashr64(long long x, int n)                         { return x >> n; }
unsigned long long shl64 (unsigned long long x, int n)                { return x << n; }
int lt_signed  (long long a, long long b)                     { return a < b; }
int lt_unsigned(unsigned long long a, unsigned long long b)   { return a < b; }

int main()
{
    int r = 0;
    r += (sdiv64(LLONG_MIN, 2) == LLONG_MIN / 2);                          // 1
    r += (udiv64(0x8000000000000000ull, 2) == 0x4000000000000000ull);     // 1 (unsigned)
    r += (ashr64(-1LL, 63) == -1LL);                                       // 1 (all ones)
    r += (shl64(1ull, 63) == 0x8000000000000000ull);                       // 1
    r += lt_signed(-1LL, 1LL);                                             // 1 (-1 < 1)
    r += (lt_unsigned((unsigned long long)-1LL, 1ull) == 0);               // 1 (~0 not < 1)
    return r;   // 6
}
