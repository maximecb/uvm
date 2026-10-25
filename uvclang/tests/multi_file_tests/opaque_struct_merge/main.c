// This file only ever sees `struct foo` as an opaque, forward-declared type
// (via a pointer); `def.c` has the full definition. Layout::new must merge
// these as compatible (the opaque declaration and the real definition
// describe the same type) rather than treating the missing-body entry here
// as a conflict with the defined one there.
#include <stdio.h>

struct foo;
struct foo *make_foo(int v);
int foo_val(struct foo *f);

int main(void)
{
    struct foo *f = make_foo(42);
    printf("%d\n", foo_val(f));
    return foo_val(f);
}
