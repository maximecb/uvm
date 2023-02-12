#include <stdlib.h>
#include <assert.h>

void main()
{
    assert(abs(-5) == 5);
    assert(abs(7) == 7);

    srand(9000);
    rand();

    // Test the exit function
    exit(0);
}
