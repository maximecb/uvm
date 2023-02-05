#include <assert.h>

char* CHAR_DOTS[256] = 0;

void main()
{
    CHAR_DOTS['0'] = "****";
    char* dots = CHAR_DOTS['0'];
    assert(dots);
}
