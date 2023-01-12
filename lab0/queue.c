/*
 * Starter file for CS 208 Lab 0: Welcome to C
 * Adapted from lab developed at CMU by R. E. Bryant, 2017-2018
 */

/*
 * This program implements a queue supporting both FIFO and LIFO
 * operations.
 *
 * It uses a singly-linked list to represent the set of queue elements
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/*
  Create empty queue.
  Return NULL if could not allocate space.
*/
Queue *q_new()
{
    Queue *q =  malloc(sizeof(Queue));

    // TODO check if malloc returned NULL (this means space could not be allocated)
    if (q == NULL) {
      return NULL;      
    }
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    return q;
}

/* Free all storage used by queue */
void q_free(Queue *q)
{
    // What if q is NULL?
    if (q == NULL) {
      return;
    }
    // TODO free the queue nodes
    /* You'll want to loop through the list nodes until the next pointer is NULL,
     * starting at the head, freeing each node and its string. 
     * Account for an empty list (head is NULL). */

    if (q->head == NULL) {
      free(q);
    } else {
        Node *curr = q->head;
        for (int i=0; i<q->size; i++) {
          Node *temp = curr->next;
          free(curr->value); // free the string value at the Node
          free(curr); // free the Node itself
          curr = temp;
        }
        free(q); // free the queue structure
    }
}

/*
  Attempt to insert element at head of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(Queue *q, char *s)
{
    Node *newh;
    char *newv;
    // What if q is NULL?
    if (q == NULL) {
      return false;
    }
    newh = malloc(sizeof(Node)); // allocates space on a the heap for the new node
    // TODO check if malloc returned NULL
    if (newh == NULL) {
      return false;
    }
    // TODO use malloc to allocate space for the value and copy s to value
    // See the C Tutor example linked from page 4 of the writeup!
    // If this second malloc call returns NULL, you need to free newh before returning
    newv = malloc(strlen(s) + 1);
    if (newv == NULL) {
      free(newh);
      return false;
    }
    strncpy(newv, s, strlen(s) + 1);
    newh->value = newv;
    newh->next = q->head;
    q->head = newh;
    q->size++;
    
    // if the list was empty, the tail might also need updating
    if (q->size == 1) {
      q->tail = q->head;
    }
    return true;
}


/*
  Attempt to insert element at tail of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(Queue *q, char *s)
{
    // TODO implement in similar fashion to q_insert_head
    Node *newt;
    char *newv;
    // check if q is NULL
    if (q == NULL) {
      return false;
    }
    newt = malloc(sizeof(Node));
    // check if malloc returned NULL
    if (newt == NULL) {
      return false;
    }
    newv = malloc(strlen(s) + 1);
    // check if malloc returns NULL and if so free newt
    if (newv == NULL) {
      free(newt);
      return false;
    }
    strncpy(newv, s, strlen(s) + 1);
    newt->value = newv;
    if (q->tail != NULL) {
      q->tail->next = newt;
    }
    q->tail = newt;
    newt->next = NULL;
    q->size++;

    // if the list was empty, the head might also need updating
    if (q->size == 1) {
      q->head = q->tail;
    }
    return true;
}

/*
  Attempt to remove element from head of queue.
  Return true if successful.
  Return false if queue is NULL or empty.
  If sp is non-NULL and an element is removed, copy the removed string to sp
  (up to a maximum of bufsize-1 characters, plus a null terminator.)
  The space used by the list element and the string should be freed.
*/
bool q_remove_head(Queue *q, char *sp, long bufsize)
{
    // TODO check if q is NULL or empty
    if (q == NULL) {
      return false;
    }
    if (q->head == NULL) {
      return false;
    }
    // TODO if sp is not NULL, copy value at the head to sp
    // Use strncpy (http://www.cplusplus.com/reference/cstring/strncpy/)
    // bufsize is the number of characters already allocated for sp
    // Copy over bufsize - 1 characters and manually write a null terminator ('\0')
    // to the last index of sp

    if (sp != NULL) {
      strncpy(sp, q->head->value, bufsize - 1);
      sp[bufsize-1] = '\0';
    } else {
      return false;
    }

    // update q->head to remove the current head from the queue
    if (q->head->next != NULL) {
      Node *newh = q->head->next;
      free(q->head->value);
      free(q->head);
      q->head = newh;
    } else { // goes into else if we are removing the last list element
      free(q->head->value);
      free(q->head);
      q->head = NULL;
      q->tail = NULL;
    }    
    
    // if the last list element was removed, the tail might need updating
    q->size--;
    return true;
}

/*
  Return number of elements in queue.
  Return 0 if q is NULL or empty
 */
int q_size(Queue *q)
{
    // What if q is NULL?
    if (q == NULL) {
      return 0;
    }
    if (q->head == NULL) {
      return 0;
    }
    return q->size;
}

/*
  Reverse elements in queue
  No effect if q is NULL or empty
  This function should not allocate or free any list elements
  (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
  It should rearrange the existing ones.
 */
void q_reverse(Queue *q)
{
    if (q == NULL) {
      return;
    }
    if (q->head == NULL) {
      return;
    }
    
}
