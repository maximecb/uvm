#include <assert.h>
#include <stdint.h>

void main()
{
    assert(1);

    int a = 1;
    int b = 2;
    int c = a + b;
    assert(c == 3);

    assert(2 > 1);
    assert(-2 < -1);

    assert(1 << 1 == 2);
    assert(2 >> 1 == 1);
}
