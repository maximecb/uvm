#include <string.h>
#include <math.h>

// 3D vector
typedef float vec3[3];

// 4x4 matrix
// Matrices are indexed as mat[col][row]
// This is a column major ordering, like OpenGL
typedef float mat44[4][4];

// +Y axis constant
vec3 VEC3_YAXIS = {
    0.0f, 1.0f, 0.0f
};

// Identity matrix constant
mat44 MAT44_IDENT = {
    { 1.0f, 0.0f, 0.0f, 0.0f},
    { 0.0f, 1.0f, 0.0f, 0.0f},
    { 0.0f, 0.0f, 1.0f, 0.0f},
    { 0.0f, 0.0f, 0.0f, 1.0f},
};

// Convert from degrees to radians
#define degtorad(a) ((a) * (M_PI_F / 180.0f))

void vec3_add(vec3 v0, vec3 v1, vec3 out)
{
    out[0] = v0[0] + v1[0];
    out[1] = v0[1] + v1[1];
    out[2] = v0[2] + v1[2];
}

// Compute the difference between two vectors
void vec3_sub(vec3 a, vec3 b, vec3 result)
{
    result[0] = a[0] - b[0];
    result[1] = a[1] - b[1];
    result[2] = a[2] - b[2];
}

// Compute the length of a vector
float vec3_length(vec3 v)
{
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

// Normalize a vector
void vec3_normalize(vec3 v)
{
    float len = vec3_length(v);
    if (len != 0.0f)
    {
        v[0] = v[0] / len;
        v[1] = v[1] / len;
        v[2] = v[2] / len;
    }
}

// Compute the dot product of two vectors
float vec3_dot(vec3 a, vec3 b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// Compute the cross product of two vectors
void vec3_cross(vec3 a, vec3 b, vec3 result)
{
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
}

void mat44_transpose(mat44 m)
{
    for (int i = 0; i < 4; ++i)
    {
        for (int j = i + 1; j < 4; ++j)
        {
            float temp = m[i][j];
            m[i][j] = m[j][i];
            m[j][i] = temp;
        }
    }
}

void mat44_mul(mat44 a, mat44 b, mat44 result)
{
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            result[i][j] = 0.0f;
            for (int k = 0; k < 4; ++k)
            {
                result[i][j] = result[i][j] + a[i][k] * b[k][j];
            }
        }
    }
}

// Generate a translation matrix
void mat44_translate(vec3 v, mat44 result)
{
    // Set the matrix to the identity
    memcpy(result, MAT44_IDENT, sizeof(mat44));

    // Set the translation components
    result[3][0] = v[0];
    result[3][1] = v[1];
    result[3][2] = v[2];
}

// Matrix for a rotation about the Y axis
void mat44_roty(float theta, mat44 result)
{
    // Set the matrix to the identity
    memcpy(result, MAT44_IDENT, sizeof(mat44));

    float cost = cosf(theta);
    float sint = sinf(theta);

    // First column
    result[0][0] = cost;
    result[0][2] = -sint;

    // Third column
    result[2][0] = sint;
    result[2][2] = cost;
}

// Transform a 3D point using a 4x4 transformation matrix
void mat44_transform(mat44 mat, vec3 in, vec3 out)
{
    out[0] = mat[0][0] * in[0] + mat[1][0] * in[1] + mat[2][0] * in[2] + mat[3][0];
    out[1] = mat[0][1] * in[0] + mat[1][1] * in[1] + mat[2][1] * in[2] + mat[3][1];
    out[2] = mat[0][2] * in[0] + mat[1][2] * in[1] + mat[2][2] * in[2] + mat[3][2];
    float w = mat[0][3] * in[0] + mat[1][3] * in[1] + mat[2][3] * in[2] + mat[3][3];

    if (w != 0.0f)
    {
        float inv_w = 1.0f / w;
        out[0] = out[0] * inv_w;
        out[1] = out[1] * inv_w;
        out[2] = out[2] * inv_w;
    }
}

// FIXME: we can't have local array variables
/*
void lookat(vec3 eye, vec3 target, vec3 up, mat44 result)
{
    vec3 forward;
    vec3 right;
    vec3 new_up;

    // Compute the forward vector
    vec3_sub(target, eye, forward);
    vec3_normalize(forward);

    // Compute the right vector
    vec3_cross(forward, up, right);
    vec3_normalize(right);

    // Compute the new up vector
    vec3_cross(right, forward, new_up);

    // Build the lookat matrix
    result[0][0] = right[0];
    result[1][0] = right[1];
    result[2][0] = right[2];
    result[3][0] = -vec3_dot(right, eye);

    result[0][1] = new_up[0];
    result[1][1] = new_up[1];
    result[2][1] = new_up[2];
    result[3][1] = -vec3_dot(new_up, eye);

    result[0][2] = -forward[0];
    result[1][2] = -forward[1];
    result[2][2] = -forward[2];
    result[3][2] = vec3_dot(forward, eye);

    result[0][3] = 0.0f;
    result[1][3] = 0.0f;
    result[2][3] = 0.0f;
    result[3][3] = 1.0f;
}
*/

// Generate a perspective projection matrix
// This is used to project coordinates from eye space into clip space.
// We follow OpenGL conventions.
//
// aspect = screen width / screen height
// fovy is in radians
//
// +Y points up,
// +X points to the right,
// -Z points into the screen.
//
// Points are projected such that X/Y are in [-1, 1]
// Z values between [near, far] get projected into [-1, 1]
// Points that are behind the camera should be discarded ahead of time
//
void perspective(float fovy, float aspect, float near, float far, mat44 result)
{
    float f = 1.0f / tanf(fovy * 0.5f);
    float nf = 1.0f / (near - far);

    result[0][0] = f / aspect;
    result[0][1] = 0.0f;
    result[0][2] = 0.0f;
    result[0][3] = 0.0f;

    result[1][0] = 0.0f;
    result[1][1] = f;
    result[1][2] = 0.0f;
    result[1][3] = 0.0f;

    result[2][0] = 0.0f;
    result[2][1] = 0.0f;
    result[2][2] = (far + near) * nf;
    result[2][3] = -1.0f;

    result[3][0] = 0.0f;
    result[3][1] = 0.0f;
    result[3][2] = 2.0f * far * near * nf;
    result[3][3] = 0.0f;
}
