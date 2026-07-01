// Pointer difference where the element stride is not a power of two, so the
// compiler divides by the stride rather than shifting. Complements ptr_bounds.c
// (which uses power-of-two int/char strides).

struct vec3 { int x, y, z; };   // 12 bytes
static struct vec3 pts[8];

long span(const struct vec3 *a, const struct vec3 *b) { return b - a; }

int main()
{
    long d1 = span(&pts[0], &pts[7]);   //  7
    long d2 = span(&pts[5], &pts[2]);   // -3
    return (int)(d1 - d2);              // 7 - (-3) = 10
}
