// Exercises global initializers: scalars, arrays, structs, strings,
// nested aggregates, and a pointer initialized from another global
// (constant getelementptr expression).

int global_int = 42;
int global_arr[5] = {1, 2, 3, 4, 5};
char small[4] = {0};
const char str[] = "world";
char *msg = "hello";

struct Point { int x, y; };
struct Point origin = {0, 0};
struct Point line[3] = {{-1, 0}, {0, 1}, {2, 3}};

int matrix[2][3] = {{1, 2, 3}, {4, 5, 6}};

int *tail = &global_arr[2];

int main()
{
    return 0;
}
