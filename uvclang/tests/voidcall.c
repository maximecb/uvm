// Void calls and calls whose result is ignored — both exercise the "discard
// the return value" (pop) path in the caller, which the other call tests never
// hit. Side effects happen through pointers and a global.

static int counter = 0;

void bump(int *p, int by) { *p += by; }              // void, side effect via ptr
void noop(void)           { }                         // void, no args, no effect
void bump_twice(int *p)   { bump(p, 1); bump(p, 1); } // void calling void
int  next_id(void)        { return ++counter; }       // non-void with a side effect

int main()
{
    int x = 10;
    bump(&x, 5);         // x = 15  (void call, nothing to bind)
    bump_twice(&x);      // x = 17
    noop();              // void call, no args
    next_id();           // counter = 1, result ignored
    next_id();           // counter = 2, result ignored
    int id = next_id();  // counter = 3, id = 3
    return x + counter + id;   // 17 + 3 + 3 = 23
}
