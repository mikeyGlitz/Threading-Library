/* 
 * File:   Queue.h
 * Author: Michael Glitzos
 *
 * Created on October 20, 2012, 9:07 PM
 * =============
 * Description:
 * =============
 * This is supposed to be a linked-list representation of a queue data structure
 * Each node consists of 2 node pointers; one points to the next node in the
 * queue, the other points to the previous node in the queue, and a pointer to
 * the thread control block that the node contains.
 * 
 * There's also methods, enqueue and dequeue, which handle adding and removing
 * items from queue.
 */
#include <stdlib.h>
#include <ucontext.h>

#ifndef QUEUE_H
#define	QUEUE_H

/* Forward declaration for thread control blocks*/
typedef struct ThrdCtrlBlk *TCB;

/* Structure for the nodes stored in a queue
 * Structure is identical to a node in a linked-list
 */
typedef struct node{
    struct node *next;          /* pointer to the next node in the list */
    struct node *previous;      /* pointer to the previous node in the list */
    TCB *contents;       /* pointer to the contents of the node */
}node;

/*
 * Structure for the queue itself
 */
typedef struct Queue{
    struct node *head; /* pointer to the head of the node */
}Queue;

/* Queue function definitions */
Queue *initialize(Queue *q);                    /* Initializes the queue */
void enqueue(Queue *q, TCB *nodeContents);     /* Add an item to the queue */
void dequeue(Queue *q, node *node);             /* Remove an item from queue */

#endif	/* QUEUE_H */

