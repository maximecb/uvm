#include <stdlib.h>
#include <assert.h>

void main()
{
    assert(abs(-5) == 5);
    assert(abs(7) == 7);

    srand(9000);

    // Rand output must be in [0, RAND_MAX]
    for (int i = 0; i < 100; ++i)
    {
        int r = rand();
        assert(r >= 0);
    }

    // Check that at least one value is positive
    while (true)
    {
        int r = rand();
        if (r > 10)
            break;
    }

    // Test the exit function
    exit(0);
}
