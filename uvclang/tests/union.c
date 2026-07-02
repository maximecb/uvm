// Unions -- untested until now, though DOOM leans on them:
//   * storage overlap / defined type-punning through a character-array member;
//   * a named union embedded in a struct, selected by a sibling tag field
//     (cf. intercept_t.d in PureDOOM.h -- ->d.thing vs ->d.line);
//   * a union of function pointers whose first member is brace-initialized in a
//     static struct-array with a function symbol cast to that member's type --
//     the exact shape of DOOM's states[] table, e.g. {(actionf_p1)A_WeaponReady}.
// Pure computation returning an exit code, diffed against native.

// (1) Overlapping storage; read the low byte written via the int member. Both
// the native reference and UVM are little-endian, so this is 0x44.
union num {
    int i;
    unsigned char bytes[4];
};

// (2) Named union embedded in a struct, DOOM intercept_t-style.
struct thing { int id; };
struct line  { int len; };
struct intercept {
    int isaline;
    union { struct thing *thing; struct line *line; } d;
};

// (3) Union of function pointers, initialized with a cast, called from a table.
typedef void (*fn_v)(void);
typedef void (*fn_p1)(void *);
typedef union { fn_p1 acp1; fn_v acv; } action_t;

static int hits;
static void bump(void *p) { hits += *(int *)p; }

struct state { int tics; action_t action; };
static int marker = 7;

static struct state states[2] = {
    { 1, {(fn_p1)bump} },
    { 2, {(fn_p1)bump} },
};

int main(void)
{
    int total = 0;

    // (1) write via .i, read the low byte via .bytes[0].
    union num n;
    n.i = 0x11223344;
    total += n.bytes[0];                 // 0x44 = 68

    // (2) named union in a struct, member chosen by the tag field.
    struct thing t = { 5 };
    struct line  l = { 9 };
    struct intercept a = { 0 }, b = { 1 };
    a.d.thing = &t;
    b.d.line  = &l;
    total += a.d.thing->id;              // +5
    total += b.d.line->len;              // +9

    // (3) call through a union fn-ptr member held in a static table.
    for (int i = 0; i < 2; i++)
        states[i].action.acp1(&marker);  // bump(&marker) twice
    total += hits;                       // 7 + 7 = +14

    return total;                        // 68 + 5 + 9 + 14 = 96
}
