// Large/sparse switch, fall-through case groups, negative and INT_MAX cases,
// and an `unreachable` reached only on a logically-impossible path.

int classify(int x)
{
    switch (x) {
        case 0:
        case 1:
        case 2:          return 10;   // fall-through group
        case 100:        return 20;
        case 1000:       return 30;
        case -5:         return 40;   // negative case
        case 2147483647: return 50;   // INT_MAX
        default:         return 99;
    }
}

int daytype(int d)
{
    switch (d) {
        case 6:
        case 7: return 0;   // weekend
        default: return 1;  // weekday
    }
}

// Every value of an i1 is covered, so the default is unreachable.
int sign_bit(int x)
{
    if ((x >> 31) & 1) return -1;
    if (!((x >> 31) & 1)) return 1;
    __builtin_unreachable();
}

int main()
{
    int r = 0;
    r += classify(1);           // 10
    r += classify(100);         // 20
    r += classify(1000);        // 30
    r += classify(-5);          // 40
    r += classify(2147483647);  // 50
    r += classify(42);          // 99
    r += daytype(6);            // 0
    r += daytype(3);            // 1
    r += sign_bit(-123);        // -1
    r += sign_bit(123);         // 1
    return r;                   // 290
}
