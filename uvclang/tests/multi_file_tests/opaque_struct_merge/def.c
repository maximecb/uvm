#include <stdlib.h>

struct foo { int v; };

struct foo *make_foo(int v)
{
    struct foo *f = malloc(sizeof(struct foo));
    f->v = v;
    return f;
}

int foo_val(struct foo *f)
{
    return f->v;
}
