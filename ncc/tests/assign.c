#include <assert.h>

u32 g = 0;

u32 foo()
{
    return 5;
}

void main()
{
    g = foo();
    assert(g == 5);
}
