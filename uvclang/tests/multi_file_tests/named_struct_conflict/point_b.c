// A genuinely different `struct Point` than point_a.c's (three doubles
// instead of two ints) that just happens to share the same name, defined
// independently (no shared header) so clang emits `%struct.Point` in both
// TUs' IR unchanged. A global instance, like point_a.c's, so the named type
// survives optimization at every -O level.
//
// Unlike an anonymous struct/union — whose clang-assigned name is a per-TU
// implementation detail with no cross-TU meaning — two *named* structs
// disagreeing under the same name is a real conflict (most likely two
// unrelated types that should never have shared a name, or an actual ODR
// violation) that uvclang must reject, not silently rename around.
struct Point {
    double x;
    double y;
    double z;
};

struct Point gb = { 1.5, 2.5, 3.0 };

double sum_b(void)
{
    return gb.x + gb.y + gb.z;
}
