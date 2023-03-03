#include <assert.h>
#include <stdint.h>

int int_array[3] = { 0, 1, 2 };

uint8_t bytes2d[2][2] = { {0, 1}, {2, 3} };

uint8_t array2d[600][800];

int main()
{
    assert(int_array[0] == 0);
    assert(int_array[1] == 1);
    assert(int_array[2] == 2);

    assert(bytes2d[0][0] == 0);
    assert(bytes2d[0][1] == 1);
    assert(bytes2d[1][0] == 2);
    assert(bytes2d[1][1] == 3);

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

    return 0;
}
