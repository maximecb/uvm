// The basic CRC-32 calculation with some optimization but no table lookup.
// The byte reversal is avoided by shifting the crc register right instead of
// left, and by using a reversed 32-bit word to represent the polynomial.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

uint32_t crc32b(const char *message)
{
    uint32_t crc = 0xFFFFFFFF;

    for (uint32_t i = 0; message[i] != 0; ++i)
    {
        crc = crc ^ (uint8_t)message[i];

        // Do eight times
        for (uint32_t j = 0; j < 8; ++j)
        {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }

    return ~crc;
}

int main(void)
{
    assert(crc32b("") == 0);
    assert(crc32b("foobar") == 2666930069u);
    assert(crc32b("One day at a time, one step at a time.") == 237905478u);

    printf("crc32(\"foobar\") = %u\n", crc32b("foobar"));
    return 0;
}
