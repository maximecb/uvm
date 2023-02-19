#include <stdint.h>
#include <assert.h>

uint32_t sdbm(unsigned char* str)
{
    uint32_t hash = 0;
    int c;

    while (c = *str)
    {
        hash = c + (hash << 6) + (hash << 16) - hash;
        ++str;
    }

    // NOTE: the bitwise AND here should not be necessary, but
    // UVM currently uses uint64 internally when doing uint32 additions,
    // so we add this to make sure we get a valid uint32 value out
    return hash & 0xFFFFFFFF;
}

void main()
{
    assert(sdbm("") == 0);
    assert(sdbm("foobar!") == 0x65a84854);
}
