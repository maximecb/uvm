#include <assert.h>

void main()
{
    // Logical negation
    assert(!0);
    assert(!!1);

    // Ternary operator
    assert(0? 0:1);
    assert(1? 1:0);

    // FIXME:
    //assert(0? 1:3 == 3);
    //assert(1? 1:3 == 1);

    // FIXME:
    assert(1 && 2);
    assert(!(1 && 0));
    assert(1 || 0);
    assert(!(0 || 0));
}
