#include <assert.h>
#include <string.h>
#include <stdint.h>

char* CHAR_DOTS[256];

char* global_str = "foobar";

uint8_t block[128];

void main()
{
    CHAR_DOTS['0'] = "****";
    char* dots = CHAR_DOTS['0'];
    assert(dots);

    assert(strlen(global_str) == 6);
    global_str = "bar";
    assert(strlen(global_str) == 3);

    // Cast array to pointer
    *(uint8_t*)block = 123;
    assert(*(uint8_t*)block == 123);
}
