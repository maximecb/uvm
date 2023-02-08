#include <assert.h>

void main()
{
    assert(0? 0:1);
    // FIXME:
    //assert(1? 1:0);
    //assert(0? 1:3 == 3);
    //assert(1? 1:3 == 1);

    // FIXME:
    //assert(1 && 2);
    assert(!(1 && 0));
    assert(1 || 0);
}
