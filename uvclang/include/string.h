#ifndef __STRING_H__
#define __STRING_H__

// <string.h> for the uvclang/UVM freestanding target. The bodies here are plain,
// standard-signature C that clang lowers to LLVM IR and uvclang then compiles
// like any other function -- this is what resolves calls such as @strlen without
// a native libc.
//
// This header is used ONLY for the UVM build (clang with -Iuvclang/include). The
// native reference build in the test harness deliberately does not see it and
// uses the platform's own libc, so these functions are checked differentially
// against the real thing.
//
// memcpy / memset / memcmp are UVM syscalls: clang lowers the common cases to
// the llvm.memcpy / llvm.memset intrinsics (which uvclang maps to native UVM
// syscalls), and <uvm/syscalls.h> exposes them explicitly. They are
// intentionally not redefined here.

#include <stddef.h>   // size_t, NULL (clang's freestanding resource header)
#include <ctype.h>    // tolower (used by strcasecmp)

// Every definition below carries weak linkage (UVCLANG_WEAK) so this header can
// be #included from any number of translation units without producing
// duplicate-symbol errors when they are linked together (LLVM keeps a single
// copy of each). No per-file "implementation" opt-in is needed; in a
// single-translation-unit build the attribute has no effect and uvclang ignores
// it.
#ifndef UVCLANG_WEAK
#define UVCLANG_WEAK __attribute__((weak))
#endif

UVCLANG_WEAK size_t strlen(const char *s)
{
    size_t l = 0;
    while (s[l] != 0)
        l = l + 1;
    return l;
}

UVCLANG_WEAK int strcmp(const char *a, const char *b)
{
    const unsigned char *ua = (const unsigned char *)a;
    const unsigned char *ub = (const unsigned char *)b;

    size_t i = 0;
    while (ua[i] != 0 && ua[i] == ub[i])
        ++i;

    return (int)ua[i] - (int)ub[i];
}

UVCLANG_WEAK int strncmp(const char *a, const char *b, size_t num)
{
    const unsigned char *ua = (const unsigned char *)a;
    const unsigned char *ub = (const unsigned char *)b;

    for (size_t i = 0; i < num; ++i)
    {
        if (ua[i] != ub[i])
            return (int)ua[i] - (int)ub[i];
        if (ua[i] == 0)
            break;
    }

    return 0;
}

// Case-insensitive string comparison. Non-standard, but widely provided.
UVCLANG_WEAK int strcasecmp(const char *a, const char *b)
{
    for (size_t i = 0;; ++i)
    {
        int ch_a = tolower((unsigned char)a[i]);
        int ch_b = tolower((unsigned char)b[i]);

        if (ch_a != ch_b)
            return ch_a - ch_b;
        if (ch_a == 0)
            break;
    }

    return 0;
}

UVCLANG_WEAK char *strchr(const char *str, int c)
{
    char ch = (char)c;

    for (;; ++str)
    {
        if (*str == ch)
            return (char *)str;
        if (*str == 0)
            return NULL;
    }
}

// Like strchr, but returns the *last* occurrence. The terminating NUL is part of
// the string, so strrchr(s, '\0') returns a pointer to it (standard behavior).
UVCLANG_WEAK char *strrchr(const char *str, int c)
{
    char ch = (char)c;
    const char *last = NULL;

    for (;; ++str)
    {
        if (*str == ch)
            last = str;
        if (*str == 0)
            return (char *)last;
    }
}

// Scan the first `num` bytes of `ptr` for byte `c`, returning a pointer to it or
// NULL. Bytes are compared as unsigned char, per the standard.
UVCLANG_WEAK void *memchr(const void *ptr, int c, size_t num)
{
    const unsigned char *p = (const unsigned char *)ptr;
    unsigned char ch = (unsigned char)c;

    for (size_t i = 0; i < num; ++i)
    {
        if (p[i] == ch)
            return (void *)(p + i);
    }

    return NULL;
}

// Returns a pointer to the first occurrence of s2 in s1, or NULL if s2 is not
// part of s1. An empty needle matches at the start of the haystack.
UVCLANG_WEAK char *strstr(const char *s1, const char *s2)
{
    while (*s1)
    {
        const char *p1 = s1;
        const char *p2 = s2;

        while (*p2 && (*p1 == *p2))
        {
            ++p1;
            ++p2;
        }

        if (!*p2)
            return (char *)s1;

        ++s1;
    }

    return *s2 ? NULL : (char *)s1;
}

UVCLANG_WEAK char *strcpy(char *dst, const char *src)
{
    char *ret = dst;

    while ((*dst = *src) != 0)
    {
        ++dst;
        ++src;
    }

    return ret;
}

UVCLANG_WEAK char *strncpy(char *dst, const char *src, size_t num)
{
    char *ret = dst;

    while (num && *src)
    {
        *dst = *src;
        ++src;
        ++dst;
        --num;
    }

    // Pad the rest with zeros until num characters have been written
    while (num > 0)
    {
        *dst = '\0';
        ++dst;
        --num;
    }

    return ret;
}

UVCLANG_WEAK char *strcat(char *dst, const char *src)
{
    char *ret = dst;

    while (*dst)
        ++dst;

    while ((*dst = *src) != 0)
    {
        ++dst;
        ++src;
    }

    return ret;
}

UVCLANG_WEAK char *strncat(char *dst, const char *src, size_t num)
{
    char *ret = dst;

    while (*dst)
        ++dst;

    while (num && *src)
    {
        *dst = *src;
        ++src;
        ++dst;
        --num;
    }

    *dst = '\0';

    return ret;
}

#endif // __STRING_H__
