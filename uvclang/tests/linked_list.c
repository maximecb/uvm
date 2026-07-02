// Self-referential, forward-declared struct tags: a doubly-linked list built
// from `struct node_s *` fields inside `struct node_s` itself -- the shape of
// DOOM's thinker_t / mobj_t lists (PureDOOM.h:920, :4187). structs.c covers
// *nested* structs; this covers a pointer to the incomplete tag at the point of
// its own definition, plus list traversal (both directions) and in-place
// mutation via relinking. A fixed pool stands in for malloc so the test stays
// pure computation diffed against native.

typedef struct node_s {
    int value;
    struct node_s *prev;     // pointer to the tag currently being defined
    struct node_s *next;
} node_t;

static node_t pool[5];

// Doubly link pool[0..n-1] and return the head.
static node_t *build(int n)
{
    for (int i = 0; i < n; i++) {
        pool[i].value = (i + 1) * (i + 1);           // 1, 4, 9, 16, 25
        pool[i].prev  = (i == 0)     ? 0 : &pool[i - 1];
        pool[i].next  = (i == n - 1) ? 0 : &pool[i + 1];
    }
    return &pool[0];
}

// Remove a node by relinking its neighbours.
static void unlink_node(node_t *n)
{
    if (n->prev) n->prev->next = n->next;
    if (n->next) n->next->prev = n->prev;
}

int main(void)
{
    node_t *head = build(5);

    int fwd = 0;
    for (node_t *p = head; p; p = p->next)
        fwd += p->value;                             // 1+4+9+16+25 = 55

    // Walk to the tail, then sum backwards to check the prev links.
    node_t *tail = head;
    while (tail->next) tail = tail->next;
    int bwd = 0;
    for (node_t *p = tail; p; p = p->prev)
        bwd += p->value;                             // 55 again

    unlink_node(&pool[2]);                           // drop value 9
    int after = 0;
    for (node_t *p = head; p; p = p->next)
        after += p->value;                           // 55 - 9 = 46

    return fwd + bwd - after;                        // 55 + 55 - 46 = 64
}
