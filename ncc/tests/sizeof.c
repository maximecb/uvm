#include <assert.h>
#include <stdint.h>

uint8_t arr10[10];
uint8_t arr2d[10][20];

void main()
{
    assert(sizeof(char) == 1);
    assert(sizeof(unsigned char) == 1);
    assert(sizeof(short) == 2);
    assert(sizeof(int) == 4);
    assert(sizeof(long) == 8);

    assert(sizeof(int*) == 8);
    assert(sizeof(int *) == 8);

    assert(sizeof(arr10) == 10);
    assert(sizeof(arr2d) == 200);
}
