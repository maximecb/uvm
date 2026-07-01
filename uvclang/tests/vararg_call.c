// Caller-side varargs: call a variadic function that never consumes its
// variadic arguments, so the callee body needs no va_list / va_arg. This
// exercises the caller pushing fixed + variadic args and the callee reading
// only the fixed one. (Callee-side va_arg is out of scope; see variadic.c.)

int pick(int a, ...) { return a * 3; }

int main()
{
    return pick(14, 1, 2, 3)   // 42
         + pick(5, 100);       // 15
}
