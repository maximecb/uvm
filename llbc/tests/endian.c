// Type punning through pointer casts: the byte order of stored integers must
// match native (little-endian). Exercises mixed-width load/store at one address
// (i32/i64 store, i8 loads at offsets, i8 store, i32 reload).

int main()
{
    unsigned int       w = 0x11223344u;
    unsigned long long q = 0x0102030405060708ull;

    unsigned char *wb = (unsigned char *)&w;
    unsigned char *qb = (unsigned char *)&q;

    int r = 0;
    // Little-endian: the lowest address holds the least-significant byte.
    r += (wb[0] == 0x44 && wb[1] == 0x33 && wb[2] == 0x22 && wb[3] == 0x11);  // 1
    r += (qb[0] == 0x08 && qb[7] == 0x01);                                    // 1

    // Reassemble the 32-bit value from its bytes.
    unsigned int reassembled =
        wb[0] | (wb[1] << 8) | (wb[2] << 16) | ((unsigned)wb[3] << 24);
    r += (reassembled == w);                                                  // 1

    // Overwrite the low byte through the char pointer, read the int back.
    wb[0] = 0xFF;
    r += (w == 0x112233FFu);                                                  // 1

    return r;   // 4
}
