#include <assert.h>
#include <stdint.h>

uint8_t array2d[600][800];

int main()
{
    /*
    assert(sizeof(array[0][0]) == 1);
    assert(sizeof(array[0]) == 800);

    assert(&(array[0][0]) + 1 == &(array[0][1]));

    void* p0 = &(array[0]);
    void* p1 = &(array[1]);
    assert(p0 + 800 == p1);
    */

    // Check row address computation
    uint8_t* row0 = *(array2d + 0);
    uint8_t* row1 = *(array2d + 1);
    assert(row1 == row0 + 800);

    // Double array indexing
    array2d[0][0] = 1;
    assert(array2d[0][0] == 1);
}
