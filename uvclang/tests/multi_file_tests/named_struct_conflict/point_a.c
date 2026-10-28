// This file's own `struct Point`: two 32-bit ints. A *global* instance
// (not a local) so the named type survives optimization at every -O level,
// including uvclang's default -O2 (a purely-local struct this small gets
// optimized away entirely and would never reach `struct_types`).
struct Point {
    int x;
    int y;
};

struct Point ga = { 3, 4 };

int sum_a(void)
{
    return ga.x + ga.y;
}
