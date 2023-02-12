#include <assert.h>
#include <stdint.h>

int global_int = 0;
uint64_t global_u64 = 0;

void main()
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

    int a = 1;
    int b = 2;
    int c = a + b;
    assert (c == 3);

    assert(2 > 1);
    assert(-2 < -1);

    // Left and right shift
    assert(1 << 1 == 2);
    assert(2 >> 1 == 1);
}
