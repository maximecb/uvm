// A string hashing function with a good distribution. Created for the sdbm
// public domain database library, used in Berkeley DB and elsewhere.

#include <stdint.h>
#include <assert.h>
#include <stdio.h>

uint32_t sdbm(const char *str)
{
    uint32_t hash = 0;
    int c;

    while ((c = (unsigned char)*str) != 0)
    {
        hash = c + (hash << 6) + (hash << 16) - hash;
        ++str;
    }

    return hash;
}

int main(void)
{
    assert(sdbm("") == 0);
    assert(sdbm("foobar!") == 0x65a84854);

    printf("sdbm(\"foobar!\") = 0x%08x\n", sdbm("foobar!"));
    return 0;
}
