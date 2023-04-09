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

// Linked-list node
typedef struct {
    int v;
    node* next;
} node;

int main()
{
    assert(sizeof(my_int) == 4);
    assert(sizeof(vec3) == 12);
    assert(sizeof(node) == 16);
    assert(sizeof(g) == 4);
    assert(sizeof(g2) == 4);
    assert(sizeof(ptr_to_vec) == sizeof(void*));




    return 0;
}
