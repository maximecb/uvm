#include <assert.h>
#include <stdlib.h>

typedef int my_int;

typedef struct {
    float x;
    float y;
    float z;
} vec3;

int g = 3;

int main()
{
    assert(sizeof(my_int) == 4);
    assert(sizeof(vec3) == 12);
    assert(sizeof(g) == 4);




    return 0;
}
