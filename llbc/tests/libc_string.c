// Differential test for <string.h>: llbc's UVM-side libc (implemented over UVM
// primitives) vs the platform's native libc. The UVM build resolves <string.h>
// to llbc/include/string.h; the native reference build uses the system header.
//
// All inputs are derived from a volatile seed so -O2 can't const-fold the calls
// away -- the string functions actually run. Comparison functions are used only
// through their sign, which both implementations agree on. The result is folded
// into one exit code compared against native.
#include <string.h>

int main()
{
    volatile int seed = 3;
    int n = seed;                       // 3

    char buf[32];
    // "aaa" of length n via strncpy from a longer source, then measure it.
    const char *src = "aaaaaaaaXXXX";
    strncpy(buf, src, (size_t)n);       // copies "aaa"
    buf[n] = '\0';

    int r = 0;
    r += (int)strlen(buf);              // 3
    r += (int)strlen("hello, world");   // 12 (runs strlen on a real string)

    // strcmp / strncmp / strcasecmp: use only the sign.
    r += (strcmp(buf, "aaa") == 0);     // 1
    r += (strcmp("abc", "abd") < 0);    // 1
    r += (strcmp("abd", "abc") > 0);    // 1
    r += (strncmp("abcXYZ", "abcQRS", 3) == 0); // 1
    r += (strcasecmp("HeLLo", "hello") == 0);   // 1

    // strchr: found and not-found.
    const char *hay = "the quick brown fox";
    r += (strchr(hay, 'q') != NULL);    // 1
    r += (strchr(hay, 'z') == NULL);    // 1
    r += (int)(strchr(hay, 'q') - hay); // 4

    // strstr: found and not-found.
    r += (strstr(hay, "brown") != NULL);        // 1
    r += (strstr(hay, "purple") == NULL);       // 1
    r += (int)(strstr(hay, "fox") - hay);       // 16

    // strncat onto the strncpy'd buffer.
    strncat(buf, "BB", 2);              // buf -> "aaaBB"
    r += (int)strlen(buf);             // 5

    // r = 3 +12 +1 +1 +1 +1 +1 +1 +1 +4 +1 +1 +16 +5 = 49
    return r;
}
