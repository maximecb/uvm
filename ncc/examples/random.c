#include <stdlib.h>
#include <uvm/syscalls.h>

void main()
{
    srand(9000);

    int N = 50;

    int min_val = 0xFFFF;
    int max_val = 0;
    int sum = 0;

    for (int i = 0; i < N; ++i)
    {
        int r = rand() % 10;
        sum = sum + r;

        print_i64(r);
        print_endl();
    }

    int avg = sum / N;
    print_str("avg: ");
    print_i64(avg);
    print_endl();
}
