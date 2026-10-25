// `shared` here is a private (static) *global variable*. `b.c` defines a
// private *function* also named `shared` — a name collision across the two
// distinct symbol tables (data vs. code) that multi-TU codegen must detect
// and rename, not just same-kind (global/global or function/function)
// collisions within one table.
static int shared = 42;

int get_shared(void)
{
    return shared;
}
