#include <assert.h>

u32 g = 555;

u32 foo()
{
    int r = 2;
    return 0;
}

void main()
{
    g = foo();
    assert(g == 0);
}
