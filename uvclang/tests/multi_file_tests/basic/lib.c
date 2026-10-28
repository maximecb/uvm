#include "lib.h"

#include <stdio.h>

static int func()
{
    return 99;
}

int DoStuff(int x)
{
    printf("Hello from lib %d\n", func());
    return x + 3;
}