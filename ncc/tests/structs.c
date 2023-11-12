#include <assert.h>

typedef struct
{
    char buf[32];
} bufstruct_t;


// Struct with an array field inside
typedef struct
{
    int user_id;
    char name[32];
} user_t;

user_t users[8];

int main()
{
    bufstruct_t* s;
    assert(sizeof(s->buf) == 32);

    // Member op
    users[0].user_id = 555;
    assert(users[0].user_id == 555);

    return 0;
}
