#include <assert.h>

/* We want the extra information from these definitions */
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */
#include <ucontext.h>
#include "ULT.h"


/* Create current thread and queue items */
static ThrdCtlBlk *currentTCB = NULL;
struct listNode *head = NULL;
static int initialized = 0;
/* So far they're just empty pointers */

/*
 * Function for initializing the threads
 */
void init(){
    /* Create a queue and a current Thread control block*/
    head = malloc(sizeof(listNode));
    head->contents = NULL; /* The contens of head always starts out as nothing */
    head->previous = head;
    head->next= head;
    
    currentTCB = malloc(sizeof(ThrdCtlBlk));
    currentTCB->my_context = malloc(sizeof(ucontext_t));
    currentTCB->my_tid = 0;
    
    initialized = 1;
}

/**
 * Function for creating threads
 * @param *fn:          Pointer to root function
 * @param *parg:        Arguments to pass into the function
 */
Tid ULT_CreateThread(void (*fn)(void *), void *parg)
{
    assert(0); /* TBD */
    return ULT_FAILED;
}

Tid ULT_SWITCH(Tid tid){
    /* capture the current context */
/*
    int curContext = getcontext(currentTCB->my_context);
*/
/*
    if(curContext != 0){
        printf("Could not properly capture thread context state\n");
    }
*/
    /* Iterate through the loop until we reach the node we're looking for*/
    listNode *currentNode = head;
    while(head->contents->my_tid != tid){
        currentNode = currentNode->next;
        if(currentNode == head)
            return ULT_NONE;
    }
    /* Perform the swap */
    assert(tid == head->contents->my_tid);
    currentTCB->my_context = head->contents->my_context;
    currentTCB->my_tid = head->contents->my_tid;
    
    head->previous->next = head->next;
    free(head->contents);
    free(head);
    
    return 1;
}

/**
 * Suspends the currently running thread and swaps it for the thread in the 
 * ready queue specified by the identifier tid
 * @param wantTid: Thread ID of the requested thread.
 * ULT_ANY tells the system to execute any thread on the ready queue
 * ULT_SELF tells the system to resume execution of the caller
 * ULT_INVALID tells the system to invoke a tid that does not correspond to a
 * valid thread
 * ULT_NONE there are no more threads to run
 * @return Thread status
 */
Tid ULT_Yield(Tid wantTid)
{
    /* Check to see if initialized */
    if(!initialized){
        init(); /* Initialize thread and queue */
    }
    /* Get real tids */
    if(wantTid == ULT_SELF){
        wantTid = currentTCB->my_tid;
    }
    else if(wantTid == ULT_ANY){
        /* if the queue is empty return ULT_None */
        if(head == NULL){
            return ULT_NONE;
        }
        else{
            /* queue contains something set wantTid to the tid of head's contents */
            wantTid = head->contents->my_tid;
        }
    }
    /* Check to see if wantTid is referencing an invalid tid */
    else if((wantTid < -2) || (wantTid > ULT_MAX_THREADS)){
        return ULT_INVALID;
    }
    /* head is current */
    head->contents = currentTCB;
    /* Perform the swap operation */
    return ULT_SWITCH(wantTid);
}

/**
 * Function to handle garbage disposal of threads
 * @param tid: Id for the thread
 * @return 
 */
Tid ULT_DestroyThread(Tid tid)
{
  assert(0); /* TBD */
  return ULT_FAILED;
}





