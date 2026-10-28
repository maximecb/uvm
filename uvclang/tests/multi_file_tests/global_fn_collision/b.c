#include <stdio.h>

// Private function sharing its name with a.c's private global `shared`. Both
// are individually valid C (distinct translation units, internal linkage);
// the cross-TU codegen merge must rename one so they don't collide in UVM's
// flat label namespace.
static void shared(void)
{
    puts("shared function called");
}

int get_shared(void);

int main(void)
{
    shared();
    printf("shared global = %d\n", get_shared());
    return 0;
}
