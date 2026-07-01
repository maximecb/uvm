// Calls into the C library strlen(), alongside manual string loops and
// pointer arithmetic (pointer difference -> ptrtoint/sub). For the UVM build
// strlen() resolves to llbc's own <string.h> (implemented over UVM primitives);
// the native reference build uses the platform libc, so this is a differential
// check of the two implementations.
#include <string.h>

// Count occurrences of a character. Uses the library strlen() on an opaque
// pointer argument, so the call is not constant-folded away.
int count_char(const char *s, char c)
{
    int n = 0;
    unsigned long len = strlen(s);
    for (unsigned long i = 0; i < len; i++)
        if (s[i] == c)
            n++;
    return n;
}

// A hand-written strlen, for comparison: pointer walk + pointer difference.
unsigned long my_strlen(const char *s)
{
    const char *p = s;
    while (*p)
        p++;
    return (unsigned long)(p - s);
}

// Which string is longer? Two library strlen() calls on opaque pointers.
int longer(const char *a, const char *b)
{
    return strlen(a) > strlen(b);
}

int main()
{
    const char *msg = "hello, world";
    return (int)my_strlen(msg)
         + count_char(msg, 'l')
         + longer("abc", "de");
}
