#include <string.h>
#include <stddef.h>
#include <assert.h>

void main()
{
    assert(strlen("") == 0);
    assert(strlen("foo") == 3);
    assert(strlen("foo" "bar") == 6);

    // FIXME: macro expansion bug
    //assert(strlen(")") == 1);

    assert(strcmp("", "") == 0);
    assert(strcmp("bar", "bar") == 0);
    assert(strcmp("bar", "fooo") == -1);
    assert(strcmp("bar", "fooo") == -1);
    assert(strcmp("foo", "fooo") == -1);
    assert(strcmp("fooo", "foo") == 1);
}
