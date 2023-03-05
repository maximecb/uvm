/*
A string hashing function with a good distribution.
Created for the sdbm public domain database library,
used in Berkeley DB and elsewhere.
*/

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

    return hash;
}

void main()
{
    assert(sdbm("") == 0);
    assert(sdbm("foobar!") == 0x65a84854);
}
