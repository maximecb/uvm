#include <assert.h>
#include <string.h>

void big_alloc()
{
    int arr[2048];
    //memset(arr, 777, 2048);
    arr[0] = 333;
    arr[2047] = 1337;
}

void multi_alloc()
{
    int arr_a[8];
    arr_a[0] = 1;
    arr_a[7] = 1;

    int arr_b[8];
    arr_b[0] = 2;
    arr_b[7] = 2;

    if (1)
    {
        int arr_c[8];
        arr_c[0] = 3;
        arr_c[7] = 3;
    }

    // Make sure this doesn't corrupt our allocs
    big_alloc();

    assert(arr_a[0] == 1);
    assert(arr_a[7] == 1);
    assert(arr_b[0] == 2);
    assert(arr_b[7] == 2);
}

int main()
{
    int arr[3];
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    assert(arr[0] == 1);
    assert(arr[1] == 2);
    assert(arr[2] == 3);

    // Make sure that a big alloc doesn't trigger a stack overflow
    for (int i = 0; i < 2048; ++i)
    {
        big_alloc();
    }

    multi_alloc();





    // Make sure that a big alloc doesn't trigger a stack overflow
    for (int i = 0; i < 2048; ++i)
    {
        big_alloc();
        multi_alloc();
    }

    return 0;
}
