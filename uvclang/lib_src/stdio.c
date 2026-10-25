#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <uvm/syscalls.h>

// Should be per thread, to be fix.
static int __uvclang_error;
int * __error(void)
{
    return &__uvclang_error;
}

static FILE __uvclang_files[FOPEN_MAX];

// Open `path` per the C mode string. The first character selects the primary
// mode (r/w/a); a '+' anywhere adds the opposite access; 'b' is ignored (all
// UVM I/O is binary). Returns a stream, or NULL on a bad mode, a rejected/absent
// path, or an exhausted stream pool.
FILE *fopen(const char *path, const char *mode)
{
    uint64_t flags;
    int append = 0;
    switch (mode[0]) {
        case 'r': flags = OPEN_READ; break;
        case 'w': flags = OPEN_WRITE | OPEN_CREATE | OPEN_TRUNC; break;
        case 'a': flags = OPEN_WRITE | OPEN_CREATE; append = 1; break;
        default:  return NULL;
    }
    for (int i = 1; mode[i] != '\0'; i++)
        if (mode[i] == '+')
            flags |= OPEN_READ | OPEN_WRITE;

    uint64_t handle = __uvm_file_open(path, flags);
    if (handle == 0)
        return NULL;

    // Claim a free slot (handle == 0). Fail closed if the pool is full.
    FILE *fp = NULL;
    for (int i = 0; i < FOPEN_MAX; i++) {
        if (__uvclang_files[i].__handle == 0) {
            fp = &__uvclang_files[i];
            break;
        }
    }
    if (fp == NULL) {
        __uvm_file_close(handle);
        return NULL;
    }

    fp->__handle = handle;
    fp->__eof = 0;
    fp->__error = 0;

    // Append mode starts positioned at end of file.
    if (append)
        __uvm_file_seek(handle, __uvm_file_size(handle));

    return fp;
}

// Close `stream` and return its pool slot. Returns 0 on success, EOF if the
// stream is NULL.
int fclose(FILE *stream)
{
    if (stream == NULL)
        return EOF;
    __uvm_file_close(stream->__handle);
    stream->__handle = 0;   // release the slot back to the pool
    return 0;
}

int __strto_float_helper(const char *str, char **endptr, 
    int* base, unsigned long* mantissa_val, unsigned long* mantissa_frac_val,
    int* mantissa_frac_nb_digit, long* exponent_val, int* negative)
{
    const char *s = str;

    // Leading whitespace.
    while (isspace((unsigned char)*s))
        ++s;

    // Optional sign.
    *negative = 0;
    if (*s == '+' || *s == '-')
    {
        *negative = (*s == '-');
        ++s;
    }

    // check for the base
    *base = 10;
    if (s[0]=='0' && (s[1]=='x' || s[1] == 'X'))
    {
        *base = 16;
        s += 2;
    }

    const char *digits = s;
    char* news = NULL;
    *mantissa_val = strtoull(s, &news, *base);
    s = news;

    *mantissa_frac_val = 0;
    *mantissa_frac_nb_digit = 0;
    if (*s == '.')
    {
        s++;
        *mantissa_frac_val = strtoull(s, &news, *base);
        *mantissa_frac_nb_digit = news-s;
        s = news;
    }

    *exponent_val = 0;
    if (*base == 10 && (*s == 'E' || *s == 'e') 
        || *base == 16 && (*s == 'p' || *s == 'P'))
    {
        s++;
        if (*s == '+')
            s++;
        *exponent_val = strtoll(s, &news, 10);
        s = news;
    }

    // No digits converted => endptr points at the original string.
    if (endptr != NULL)
        *endptr = (char *)((s == digits) ? str : s);
    return s != digits;
}

double strtod(const char *str, char **endptr)
{
    int neg = 0;
    int base = 0;
    unsigned long mantissa_val = 0;
    unsigned long mantissa_frac_val = 0;
    int mantissa_frac_nb_digit = 0;
    long exponent_val = 0;
    if (!__strto_float_helper(str, endptr, 
        &base, &mantissa_val, 
        &mantissa_frac_val, &mantissa_frac_nb_digit, 
        &exponent_val, &neg))
        return 0.0f;

    double val = mantissa_val + (mantissa_frac_val/pow(base, mantissa_frac_nb_digit));
    if (neg)
        val = -val;
    if (exponent_val != 0)
        val = val * pow(base==10 ? 10 : 2, exponent_val);

    return val;
}

double strtold(const char *str, char **endptr)
{
    return strtod(str, endptr);
}

float strtof(const char *str, char **endptr)
{
    int neg = 0;
    int base = 0;
    unsigned long mantissa_val = 0;
    unsigned long mantissa_frac_val = 0;
    int mantissa_frac_nb_digit = 0;
    long exponent_val = 0;
    if (!__strto_float_helper(str, endptr, 
        &base, &mantissa_val, 
        &mantissa_frac_val, &mantissa_frac_nb_digit, 
        &exponent_val, &neg))
        return 0.0f;

    float val = mantissa_val + (mantissa_frac_val/pow(base, mantissa_frac_nb_digit));
    if (neg)
        val = -val;
    if (exponent_val != 0)
        val = val * pow(base==10 ? 10 : 2, exponent_val);

    return val;
}
