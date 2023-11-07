#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

uint8_t arr[19];
uint8_t arr2[19];

char dst[32];

char* global_str = "global string pointer";

char null_ch = '\0';
char* str_with_null = "hello\0null";

int main()
{
    assert(strlen("") == 0);
    assert(strlen("foo") == 3);
    assert(strlen("foo" "bar") == 6);
    assert(strlen("()") == 2);
    assert(strlen(")") == 1);
    assert(global_str);
    assert(strlen(global_str) == 21);

    assert(strcmp("", "") == 0);
    assert(strcmp("bar", "bar") == 0);
    assert(strcmp("bar", "fooo") == -1);
    assert(strcmp("bar", "fooo") == -1);
    assert(strcmp("foo", "fooo") == -1);
    assert(strcmp("fooo", "foo") == 1);

    assert(strncmp("", "foobar", 0) == 0);
    assert(strncmp("foo", "foobar", 3) == 0);
    assert(strncmp("foobar", "foobar", 6) == 0);

    // strchr
    char* str = "lestring";
    assert(strchr("", 'c') == NULL);
    assert(strchr("c", 'c') != NULL);
    assert(strchr(str, 's') == str + 2);

    // strstr
    char* s = "abcabcabcdabcde";
    assert(strstr(s, "x") == NULL);
    assert(strstr(s, "xyz") == NULL);
    assert(strstr(s, "a") == s + 0);
    assert(strstr(s, "abc") == s + 0);
    assert(strstr(s, "abcd") == s + 6);
    assert(strstr(s, "abcde") == s + 10);

    // strncat
    assert(strncat(dst, "foo", 3) == dst);
    assert(strcmp(dst, "foo") == 0);
    assert(strncat(dst, "bar", 3) == dst);
    assert(strcmp(dst, "foobar") == 0);

    // strncpy
    memset(dst, 0, sizeof(dst));
    assert(strncpy(dst, "foo", 3) == dst);
    assert(strcmp(dst, "foo") == 0);
    assert(strncpy(dst, "", 3) == dst);
    assert(strcmp(dst, "") == 0);

    // memset
    memset(arr, 177, 19);
    assert(arr[0] == 177);
    assert(arr[18] == 177);

    // memcpy
    memcpy(arr2, arr, 19);
    assert(arr2[0] == 177);
    assert(arr2[18] == 177);

    // From ctype.h
    assert(isprint(' '));
    assert(isprint('A'));
    assert(isupper('A'));
    assert(!isupper('a'));

    return 0;
}
