#include <assert.h>

int main()
{
    int i = 0;
    do
    {
        i = i + 1;
    } while (i < 10);
    assert(i == 10);

    // The test needs to still be executed even though
    // there is a continue in the loop body
    do
    {
        continue;
    } while (0);

    i = 0;
    do
    {
        i = i + 1;
        continue;
    } while (i < 10);
    assert(i == 10);

    return 0;
}
