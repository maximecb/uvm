#include <assert.h>
#include <stdint.h>

int global_int = 0;
uint64_t global_u64 = 0;

int main()
{
    // True/false values and logical negation
    assert(1);
    assert(!0);
    assert(8);

    // Infix operations
    assert(3 - 1 == 2);
    assert(3 * 2 - 1 == 5);
    assert(1 + 1 + 1 + 1 == 4);
    assert(1 + 3 * 2 == 7);
    assert(3 - 1 + 1 == 3);
    assert(3 - 1 - 2 == 0);

    // Unary minus
    assert(3 - -1 == 4);
    assert(4 + -1 == 3);

    // Pre-increment and pre-decrement
    int n = 1;
    ++n;
    assert(n == 2);
    --n;
    assert(n == 1);

    int a = 1;
    int b = 2;
    int c = a + b;
    assert (c == 3);

    // Signed vs unsigned comparison
    assert(2 > 1);
    assert(-2 < -1);
    assert(-1 < 3);
    assert((uint64_t)-1 == -1);
    assert(2 - 3 == -1);
    int x = 2;
    int y = 3;
    assert(x - y == -1);
    int z = x - y;
    assert(z < 0);
    assert(z <= 0);

    // Unsigned modulo
    int i = -1;
    assert((unsigned int)i % 5 == 0);

    // Left and right shift
    assert(1 << 1 == 2);
    assert(2 >> 1 == 1);

    assert((1 & 1) == 1);
    assert((3 & 1) == 1);
    assert((1 & 0) == 0);
    assert((1 ^ 1) == 0);
    assert((1 ^ 0) == 1);

    // Toggling a bit
    assert((0xFFFF ^ 16) < 0xFFFF);

    // Integer casting
    assert(((int)(uint8_t)3) == 3);

    return 0;
}
