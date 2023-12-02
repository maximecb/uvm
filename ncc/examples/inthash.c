#include <stdint.h>
#include <assert.h>
#include <stdio.h>

// Integer hash function with a good distribution
// Based on:
// https://github.com/skeeto/hash-prospector
uint32_t hash_u32(uint32_t x)
{
    x = x ^ (x >> 16);
    x = x * 0x7feb352d;
    x = x ^ (x >> 15);
    x = x * (0x846ca68b);
    x = x ^ (x >> 16);
    return x;
}

int main()
{
    // For comparison against GCC
    assert(hash_u32(0) == 0);
    assert(hash_u32(1) == 1753845952);
    assert(hash_u32(2) == 3507691905);
    assert(hash_u32(3) == 1408362973);

    uint32_t sum = 0;

    for (uint32_t i = 0; i < 30; ++i)
    {
        uint32_t r = hash_u32(i) % 20;

        #ifndef TEST
        printf("%u\n", r);
        #endif

        sum = sum + r;
    }

    #ifndef TEST
    printf("sum = %u\n", sum);
    #endif

    assert(sum == 271);

    return 0;
}
