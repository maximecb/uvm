// Signed/unsigned division and remainder with negative operands (C truncates
// toward zero; the remainder takes the sign of the dividend), the three shift
// operators, and well-defined INT_MIN edge cases. Kept free of UB: divisors
// are non-zero and never the INT_MIN/-1 overflow case, INT_MIN is never negated
// or divided by -1, and every shift count is in range. The result is summed
// into an unsigned accumulator so there is no signed-overflow UB either.

#include <limits.h>

int      sdiv(int a, int b)          { return a / b; }
int      srem(int a, int b)          { return a % b; }
unsigned udiv(unsigned a, unsigned b){ return a / b; }
unsigned urem(unsigned a, unsigned b){ return a % b; }
int      ashr(int x, int n)          { return x >> n; }   // arithmetic (sign-propagating)
unsigned lshr(unsigned x, int n)     { return x >> n; }   // logical
int      shl(int x, int n)           { return x << n; }

int main()
{
    unsigned acc = 0;
    acc += (unsigned)sdiv(-7, 2);        // -3
    acc += (unsigned)srem(-7, 2);        // -1
    acc += (unsigned)sdiv(7, -2);        // -3
    acc += (unsigned)srem(7, -2);        // 1
    acc += udiv(4000000000u, 7);         // 571428571
    acc += urem(255, 4);                 // 3
    acc += (unsigned)ashr(-16, 2);       // -4
    acc += lshr(0x80000000u, 4);         // 0x08000000
    acc += (unsigned)shl(1, 10);         // 1024
    // Well-defined INT_MIN edge cases (no INT_MIN/-1, no negation of INT_MIN).
    acc += (unsigned)sdiv(INT_MIN, 2);   // -1073741824
    acc += (unsigned)srem(INT_MIN, 7);   // -2
    acc += (unsigned)ashr(INT_MIN, 4);   // arithmetic shift of a negative
    acc += (unsigned)INT_MIN;            // 2147483648
    acc += (unsigned)(INT_MIN < 0);      // 1 (signed compare)
    return (int)(acc & 0x7F);
}
