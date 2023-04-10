#include <assert.h>
#include <stdlib.h>

int g = 3;

typedef int my_int;
my_int g2 = 4;

// Linked-list node
typedef struct {
    int v;
    node* next;
} node;

// 3D vector
typedef struct {
    float x;
    float y;
    float z;
} vec3;

// 4x4 matrix
typedef float mat44[4][4];

vec3* ptr_to_vec = NULL;

vec3 g_vec;

mat44 g_mat = {
    { 1.0f, 0.0f, 0.0f, 0.0f},
    { 0.0f, 1.0f, 0.0f, 0.0f},
    { 0.0f, 0.0f, 1.0f, 0.0f},
    { 0.0f, 0.0f, 0.0f, 1.0f},
};

void add(vec3* v0, vec3* v1, vec3* out)
{
    out->x = v0->x + v1->x;
    out->y = v0->y + v1->y;
    out->z = v0->z + v1->z;
}

void mat44_test(mat44 m)
{
    assert(m[2][2] == 1.0f);
    assert(m[2][3] == 0.0f);
}

int main()
{
    assert(sizeof(my_int) == 4);
    assert(sizeof(vec3) == 12);
    assert(sizeof(g_vec) == 12);
    assert(sizeof(node) == 16);
    assert(sizeof(g) == 4);
    assert(sizeof(g2) == 4);
    assert(sizeof(ptr_to_vec) == sizeof(void*));

    my_int n = 2;
    assert(n == 2);
    assert((my_int)3 == 3);

    (&g_vec)->z = 1.0f;
    add(&g_vec, &g_vec, &g_vec);
    assert((&g_vec)->z == 2.0f);

    mat44_test(g_mat);

    return 0;
}
