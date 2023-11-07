#include <assert.h>
#include <stdint.h>
#include <stddef.h>

int int_array[3] = { 7, 1, 2 };

uint8_t bytes2d[2][2] = { {0, 1}, {2, 3} };

uint8_t array2d[600][800];

char* str_array[3] = { "foo", "bar", "bif" };

// Integer literals of type long and int in the same array literal
uint64_t arr_int_long[2] = { 0x7fff8beb, 0x8000e82a };

int main()
{
    assert(int_array[0] == 7);
    assert(int_array[1] == 1);
    assert(int_array[2] == 2);

    assert(bytes2d[0][0] == 0);
    assert(bytes2d[0][1] == 1);
    assert(bytes2d[1][0] == 2);
    assert(bytes2d[1][1] == 3);

    assert(str_array[2][1] == 'i');

    // Regression: signed integer index needs to be sign-extended
    int* p = int_array + 2;
    int idx = 1 - 3;
    assert(p[idx] == 7);

    // Sizeof operator and arrays
    assert(sizeof(array2d) == 480000);
    assert(sizeof(array2d[0]) == 800);
    assert(sizeof(array2d[0][0]) == 1);

    // Array element addresses
    assert(&int_array[0] == int_array);
    assert(&(array2d[0][0]) + 1 == &(array2d[0][1]));
    int* p0 = &int_array[0];
    int* p1 = &int_array[1];
    assert(p0 + 1 == p1);

    // Check row address computation
    uint8_t* row0 = *(array2d + 0);
    uint8_t* row1 = *(array2d + 1);
    assert(row1 == row0 + 800);

    // Double array indexing
    array2d[0][0] = 1;
    assert(array2d[0][0] == 1);

    // Regression: sizeof of local array inside var decl
    char buf[128];
    sizeof(buf);
    size_t sz = sizeof(buf);
    assert(sz == 128);

    return 0;
}
