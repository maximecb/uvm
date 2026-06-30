// Pointers, arrays, and struct field access: getelementptr / load / store,
// plus stack allocation (alloca) of locals.

struct Vec { int x, y, z; };

int dot(struct Vec *a, struct Vec *b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

void scale(struct Vec *v, int s)
{
    v->x *= s;
    v->y *= s;
    v->z *= s;
}

int sum_array(const int *arr, int n)
{
    int s = 0;
    for (int i = 0; i < n; i++)
        s += arr[i];
    return s;
}

int nested(int (*m)[4])   // pointer to array of 4 ints
{
    return m[1][3] + m[0][0];
}

int main()
{
    struct Vec a = {1, 2, 3}, b = {4, 5, 6};
    scale(&a, 2);
    int arr[5] = {10, 20, 30, 40, 50};
    int grid[2][4] = {{1, 2, 3, 4}, {5, 6, 7, 8}};
    return dot(&a, &b) + sum_array(arr, 5) + nested(grid);
}
