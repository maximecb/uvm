#include <assert.h>

int arr[16];

int foo()
{
    return 0;
}

int* bar()
{
    return arr;
}

int main()
{
    // Logical negation
    assert(!0);
    assert(!!1);

    // Comma sequencing operator
    assert((1, 2) == 2);

    // Ternary operator
    assert(0? 0:1);
    assert(1? 1:0);
    assert((0? 1:3) == 3);
    assert((1? 1:3) == 1);
    int a = 2 & 2? 8:4;
    assert(a == 8);

    // Logical AND, OR, NOT
    assert(1 && 2);
    assert(!(1 && 0));
    assert(1 || 0);
    assert(!(0 || 0));

    // Negation of a call
    assert(!foo());

    // Prefix and postfix operator precedence
    bar()[0] = 77;
    assert(bar()[0] == 77);
    assert(!!bar()[0]);
    assert(bar()[0] + 1 == 78);

    return 0;
}
