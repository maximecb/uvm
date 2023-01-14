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

u32 crc32b(u8* message)
{
    size_t i = 0;
    u32 crc = 0xFFFFFFFF;

    while (message[i] != 0)
    {
        // Get next byte
        u8 byte = message[i];
        crc = crc ^ byte;

        // Do eight times
        for (size_t j = 0; j < 8; ++j)
        {
            u32 mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }

        i = i + 1;
    }

    return ~crc;
}

// TODO: we should create a microbenchmark out of this
void main()
{
}
