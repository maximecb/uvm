#include <stdint.h>
#include <stdio.h>

typedef struct
{
    uint32_t user_id;
    uint8_t state;

    // There should be 3 padding bytes here to make arrays possible
} user_t;

user_t users[2];

int main()
{
    // This should be 8, not 5
    assert(sizeof(user_t) == 8);
    //printf("sizeof(user_t)=%d\n", sizeof(user_t));

    // This used to fail because of an unaligned access
    user_t* p_user = &users[1];
    p_user->user_id = 777;

    return 0;
}
