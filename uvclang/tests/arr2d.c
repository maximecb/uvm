// Multi-dimensional arrays: GEP with runtime indices across two array
// dimensions (nested-array stride). Static storage avoids memcpy-style init.

static int grid[3][4] = {
    {   1,   2,   3,   4 },
    {  10,  20,  30,  40 },
    { 100, 200, 300, 400 },
};

int at(int i, int j) { return grid[i][j]; }   // two runtime array indices

int row_sum(int i)
{
    int s = 0;
    for (int j = 0; j < 4; j++)
        s += grid[i][j];
    return s;
}

int diag(void)
{
    int s = 0;
    for (int i = 0; i < 3; i++)
        s += grid[i][i];     // 1 + 20 + 300
    return s;
}

int main()
{
    return at(2, 1)      // 200
         + row_sum(1)    // 100
         + diag();       // 321
}                        // 621
