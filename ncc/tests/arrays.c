#include <assert.h>
#include <stdint.h>

int int_array[3] = { 7, 1, 2 };

uint8_t bytes2d[2][2] = { {0, 1}, {2, 3} };

uint8_t array2d[600][800];

int main()
{
    assert(int_array[0] == 7);
    assert(int_array[1] == 1);
    assert(int_array[2] == 2);

    assert(bytes2d[0][0] == 0);
    assert(bytes2d[0][1] == 1);
    assert(bytes2d[1][0] == 2);
    assert(bytes2d[1][1] == 3);

    // Regression: signed integer index needs to be sign-extended
    int* p = int_array + 2;
    int idx = 1 - 3;
    assert(p[idx] == 7);

    // Sizeof operator and arrays
    assert(sizeof(array2d) == 480000);
    assert(sizeof(array2d[0]) == 800);
    assert(sizeof(array2d[0][0]) == 1);

    /*
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
