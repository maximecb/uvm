// Self-checking test for <uvm/3dmath.h> (vec3 / mat44 f32 helpers). Runs under
// run_uvm_tests.sh: exits 0 iff every check passes. Exact IEEE results are
// asserted directly; anything through sqrtf/sinf/cosf/division uses a tolerance.
// A volatile seed keeps -O2 from const-folding the whole thing away.
#include <assert.h>
#include <uvm/3dmath.h>

static int approx(float a, float b) { return fabsf(a - b) < 1e-3f; }

int main()
{
    volatile int seed = 3;
    float s = (float)seed;              // 3.0

    vec3 a = { s, 4.0f, 0.0f };         // (3,4,0)
    vec3 b = { 1.0f, 0.0f, 0.0f };
    vec3 out;

    vec3_add(a, b, out);
    assert(out[0] == 4.0f && out[1] == 4.0f && out[2] == 0.0f);

    vec3_sub(a, b, out);
    assert(out[0] == 2.0f && out[1] == 4.0f && out[2] == 0.0f);

    assert(approx(vec3_length(a), 5.0f));          // sqrt(9+16) = 5
    assert(vec3_dot(a, b) == 3.0f);                // 3*1 + 4*0 + 0*0

    // cross(x-axis, y-axis) = z-axis
    vec3 xa = { 1.0f, 0.0f, 0.0f };
    vec3 ya = { 0.0f, 1.0f, 0.0f };
    vec3_cross(xa, ya, out);
    assert(out[0] == 0.0f && out[1] == 0.0f && out[2] == 1.0f);

    // normalize (3,4,0) -> (0.6, 0.8, 0), unit length
    vec3 n = { s, 4.0f, 0.0f };
    vec3_normalize(n);
    assert(approx(n[0], 0.6f) && approx(n[1], 0.8f) && approx(n[2], 0.0f));
    assert(approx(vec3_length(n), 1.0f));

    // identity * identity = identity
    mat44 m;
    mat44_mul(MAT44_IDENT, MAT44_IDENT, m);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            assert(m[i][j] == MAT44_IDENT[i][j]);

    // translate by (5,6,7) then transform the point (1,1,1) -> (6,7,8)
    vec3 tv = { 5.0f, 6.0f, 7.0f };
    mat44 tm;
    mat44_translate(tv, tm);
    vec3 p = { 1.0f, 1.0f, 1.0f };
    vec3 tp;
    mat44_transform(tm, p, tp);
    assert(approx(tp[0], 6.0f) && approx(tp[1], 7.0f) && approx(tp[2], 8.0f));

    // rotation by 0 radians leaves a point unchanged (exercises sinf/cosf)
    mat44 rm;
    mat44_rotx(0.0f, rm);
    vec3 rp;
    mat44_transform(rm, p, rp);
    assert(approx(rp[0], 1.0f) && approx(rp[1], 1.0f) && approx(rp[2], 1.0f));

    return 0;
}
