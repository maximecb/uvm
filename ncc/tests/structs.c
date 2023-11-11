#include <assert.h>

// Struct with an array field inside
typedef struct
{
    int user_id;
    char name[32];
} user_t;

typedef struct
{
    char buf[32];
} bufstruct_t;

int main()
{
    bufstruct_t* s;
    assert(sizeof(s->buf) == 32);

    return 0;
}
