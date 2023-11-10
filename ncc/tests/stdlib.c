#include <stdlib.h>
#include <assert.h>

char buf[32];

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
    while (1)
    {
        int r = rand();
        if (r > 10)
            break;
    }

    // Test itoa function
    itoa(5, buf, 10);
    assert(buf[0] == '5' && buf[1] == 0);
    itoa(17, buf, 10);
    assert(buf[0] == '1' && buf[1] == '7' && buf[2] == 0);
    itoa(15, buf, 16);
    assert(buf[0] == 'F' && buf[1] == 0);

    // Test the exit function
    exit(0);
}
