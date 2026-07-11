// Draw a handful of pseudo-random numbers with the standard rand()/srand() and
// print their running average.

#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    srand(9000);

    int N = 50;
    int sum = 0;

    for (int i = 0; i < N; ++i)
    {
        int r = rand() % 10;
        sum += r;
        printf("%d\n", r);
    }

    printf("avg: %d\n", sum / N);
    return 0;
}
