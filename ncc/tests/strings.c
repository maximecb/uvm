#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

uint8_t arr[19];
uint8_t arr2[19];

void main()
{
    assert(strlen("") == 0);
    assert(strlen("foo") == 3);
    assert(strlen("foo" "bar") == 6);
    assert(strlen("()") == 2);
    assert(strlen(")") == 1);

    assert(strcmp("", "") == 0);
    assert(strcmp("bar", "bar") == 0);
    assert(strcmp("bar", "fooo") == -1);
    assert(strcmp("bar", "fooo") == -1);
    assert(strcmp("foo", "fooo") == -1);
    assert(strcmp("fooo", "foo") == 1);

    // memset
    memset(arr, 177, 19);
    assert(arr[0] == 177);
    assert(arr[18] == 177);

    // memcpy
    memcpy(arr2, arr, 19);
    assert(arr2[0] == 177);
    assert(arr2[18] == 177);
}
