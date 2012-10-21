#include <assert.h>

/* We want the extra information from these definitions */
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */
#include <ucontext.h>
#include "Queue.h"
#include "ULT.h"


/* Create current thread and queue items */
static ThrdCtlBlk *currentTCB = NULL;
static Queue *ready = NULL;
/* So far they're just empty pointers */

/*
 * Function for initializing the threads
 */
void init(){
    /* Create a queue and a current Thread control block*/
    ready = malloc(sizeof(Queue));
    initializeQueue(ready);
    
    currentTCB = malloc(sizeof(ThrdCtlBlk));
    currentTCB->my_context = malloc(sizeof(ucontext_t));
    currentTCB->my_tid = 0;
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
    if(wantTid == ULT_SELF)
        wantTid = currentTCB->my_tid;
    assert(0);
    return ULT_FAILED;
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





