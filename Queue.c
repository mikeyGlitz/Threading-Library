/* 
 * Queue.c
 * 
 * ============
 * Description
 * ============
 * This file contains the implementation for the source in Queue.h.
 */
#include "Queue.h"

/* Initialize the queue */
Queue *initializeQueue(Queue *q){
    q->head = NULL;     /* We want to start out with an empty queue */
    return q;
}

void enqueue(Queue *q, TCB *nodeContents){
    if(q->head == NULL){
        q->head = malloc(sizeof(node));         /* allocate space in memory for the head node */
        q->head->contents = nodeContents;       /* set the head node to the node contents */
        /* set previous and next */
        q->head->previous = NULL;
        q->head->next = NULL;           
    }
    else{
        q->head->previous = q->head;
        q->head->next = malloc(sizeof(node));
        q->head = q->head->next;
        q->head->contents = nodeContents;
        q->head->next = NULL;
    }
}
void dequeue(Queue *q, TCB *nodeContents){
    if(q->head == NULL){
        /* cannot dequeue something that's already empty */
    }
    else{
        /* traverse the list until we reach the desired node */
        while(q->head->contents != nodeContents){
            q->head = q->head->next;
        }
        if(q->head->previous == NULL){
            /* There is only one item in the list */
            free(q->head);
        }
        else{
            /* link the previous link to the next node */
            q->head->previous->next = q->head->next;
            /* Free the memory at the current location */
            free(q->head);
            /* the current head is what used to be the previous head */
            q->head = q->head->previous;
        }
    }
}