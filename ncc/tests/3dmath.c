#include <stdio.h>
#include <uvm/math.h>
#include <uvm/3dmath.h>

mat44 persp;

vec3 v_in;
vec3 v_out;

int main()
{
    perspective(
        DEG2RAD(40.0f),
        1.0f,   // aspect,
        0.1f,   // near,
        100.0f, // far,
        persp
    );

    // -Z
    v_in[0] = 0.0f;
    v_in[1] = 0.0f;
    v_in[2] = -10.0f;
    mat44_transform(persp, v_in, v_out);
    assert(v_out[0] == 0.0f);
    assert(v_out[1] == 0.0f);
    assert(v_out[2] > 0.0f && v_out[2] < 1.0f);

    // +X
    v_in[0] = 3.0f;
    v_in[1] = 0.0f;
    v_in[2] = -10.0f;
    mat44_transform(persp, v_in, v_out);
    assert(v_out[0] > 0.0f && v_out[0] < 1.0f);
    assert(v_out[1] == 0.0f);
    assert(v_out[2] > 0.0f && v_out[2] < 1.0f);

    // +Y
    v_in[0] = 0.0f;
    v_in[1] = 3.0f;
    v_in[2] = -10.0f;
    mat44_transform(persp, v_in, v_out);
    assert(v_out[0] == 0.0f);
    assert(v_out[1] > 0.0f && v_out[1] < 1.0f);
    assert(v_out[2] > 0.0f && v_out[2] < 1.0f);

    return 0;
}
