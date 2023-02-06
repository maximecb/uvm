#include <string.h>
#include <stddef.h>
#include <assert.h>

void main()
{
    size_t l = strlen("foo");
    assert(l == 3);

    size_t l2 = strlen("foo" "bar");
    assert(l2 == 6);

    // TODO: assert output
    strcmp("bar", "foo");
}
