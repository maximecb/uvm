#include <assert.h>
#include <string.h>

char* CHAR_DOTS[256];

char* global_str = "foobar";

void main()
{
    CHAR_DOTS['0'] = "****";
    char* dots = CHAR_DOTS['0'];
    assert(dots);

    //assert(strlen(global_str) == 6);
    //global_str = "bar";
    //assert(strlen(global_str) == 3);
}
