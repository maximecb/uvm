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

    int a = 1;
    int b = 2;
    int c = a + b;
    assert (c == 3);

    assert(2 > 1);
    assert(-2 < -1);

    assert(1 << 1 == 2);
    assert(2 >> 1 == 1);
}
