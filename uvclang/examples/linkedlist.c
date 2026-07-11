// Build a small singly linked list on the heap, walk it, then free it.

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct node
{
    int v;
    struct node *next;
} node_t;

int main(void)
{
    // Push 0..9 onto the front of the list.
    node_t *list = NULL;
    for (int i = 0; i < 10; ++i)
    {
        node_t *n = (node_t *)malloc(sizeof(node_t));
        n->v = i;
        n->next = list;
        list = n;
    }

    // Walk the list, summing the values.
    int sum = 0;
    int count = 0;
    for (node_t *p = list; p != NULL; p = p->next)
    {
        sum += p->v;
        ++count;
    }

    printf("count = %d, sum = %d\n", count, sum);
    assert(count == 10);
    assert(sum == 45);

    // Free every node.
    while (list != NULL)
    {
        node_t *next = list->next;
        free(list);
        list = next;
    }

    return 0;
}
