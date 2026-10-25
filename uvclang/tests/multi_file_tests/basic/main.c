#include <stdio.h>
#include "lib.h"

static int func()
{
    return 10;
}

int main(void)
{
    printf("Hello from main %d\n", func());
    return DoStuff(10);
}