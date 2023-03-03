/*
This is the basic CRC-32 calculation with some optimization but no
table lookup. The the byte reversal is avoided by shifting the crc reg
right instead of left and by using a reversed 32-bit word to represent
the polynomial.

When compiled to Cyclops with GCC, this function executes in 8 + 72n
instructions, where n is the number of bytes in the input message. It
should be doable in 4 + 61n instructions.

If the inner loop is strung out (approx. 5*8 = 40 instructions),
it would take about 6 + 46n instructions.
*/

#include <assert.h>
#include <stdint.h>

uint32_t crc32b(uint8_t* message)
{
    uint32_t i = 0;
    uint32_t crc = 0xFFFFFFFF;

    while (message[i] != 0)
    {
        // Get next byte
        uint8_t byte = message[i];
        crc = crc ^ byte;

        // Do eight times
        for (uint32_t j = 0; j < 8; ++j)
        {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }

        ++i;
    }

    return ~crc;
}

void main()
{
    uint32_t r = crc32b("");
    assert(r == 0);

    uint32_t r2 = crc32b("foobar");
    assert(r2 == 2666930069);

    uint32_t r3 = crc32b("One day at a time, one step at a time.");
    assert(r3 == 237905478);
}
