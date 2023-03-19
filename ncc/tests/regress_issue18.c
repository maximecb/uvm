#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

size_t global_var = 1024;

uint32_t* array;

int main()
{
    // Previously, we did not load global pointer variables correctly
    array = (uint32_t*)malloc( sizeof(uint32_t) * 1024 );
    memset(array, 0, 4096);
    assert(global_var == 1024);

    return 0;
}
