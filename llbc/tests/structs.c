// Nested structs, arrays of structs, and a function pointer stored in a struct
// field. Aggregates are passed by pointer and initialized statically (per the
// ABI guideline) to avoid struct-by-value coercion and memcpy.

struct point { int x, y; };
struct rect  { struct point lo, hi; };          // nested struct

int area(const struct rect *r)                   // struct by pointer
{
    int w = r->hi.x - r->lo.x;
    int h = r->hi.y - r->lo.y;
    return w * h;
}

static struct rect rects[3] = {
    { {0, 0},   {2, 3} },       // area 6
    { {1, 1},   {4, 5} },       // area 12
    { {-2, -2}, {2, 2} },       // area 16
};

int add(int a, int b) { return a + b; }
int mul(int a, int b) { return a * b; }

struct op { int (*fn)(int, int); int tag; };     // function pointer in a struct
static struct op ops[2] = { { add, 1 }, { mul, 2 } };

int apply_ops(int a, int b)
{
    int r = 0;
    for (int i = 0; i < 2; i++)
        r += ops[i].fn(a, b) + ops[i].tag;       // indirect call through a field
    return r;
}

int main()
{
    int total = 0;
    for (int i = 0; i < 3; i++)
        total += area(&rects[i]);   // 6 + 12 + 16 = 34
    total += apply_ops(3, 4);       // (7 + 1) + (12 + 2) = 22
    return total;                   // 56
}
