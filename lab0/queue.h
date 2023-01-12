/* Linked list element */
typedef struct node {
    char *value;
    struct node *next;
} Node;

/* Queue structure */
typedef struct {
    Node *head; /* Linked list of elements */
    Node *tail;
    long size;
} Queue;