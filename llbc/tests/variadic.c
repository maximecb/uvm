// Variadic function using <stdarg.h> (a freestanding clang header).

#include <stdarg.h>

int sum_args(int count, ...)
{
    va_list ap;
    va_start(ap, count);
    int total = 0;
    for (int i = 0; i < count; i++)
        total += va_arg(ap, int);
    va_end(ap);
    return total;
}

int main()
{
    return sum_args(4, 10, 20, 30, 40);  // 100
}
