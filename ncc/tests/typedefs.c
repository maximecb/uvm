#include <assert.h>
#include <stdlib.h>

int g = 3;

typedef int my_int;
my_int g2 = 4;

// 3D vector
typedef struct {
    float x;
    float y;
    float z;
} vec3;

vec3* ptr_to_vec = NULL;

vec3 global_vec;

// Linked-list node
typedef struct {
    int v;
    node* next;
} node;

void add(vec3* v0, vec3* v1)
{
}

int main()
{
    assert(sizeof(my_int) == 4);
    assert(sizeof(vec3) == 12);
    assert(sizeof(node) == 16);
    assert(sizeof(g) == 4);
    assert(sizeof(g2) == 4);
    assert(sizeof(ptr_to_vec) == sizeof(void*));

    my_int n = 2;
    assert(n == 2);
    assert((my_int)3 == 3);

    return 0;
}
