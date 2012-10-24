#include <assert.h>

/* We want the extra information from these definitions */
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */
#include <ucontext.h>
#include "ULT.h"

/* Checktools */
static ThrdCtlBlk *notFound;
static ThrdCtlBlk *noneLeft; 

/* Create current thread and queue items */
static ThrdCtlBlk *currentTCB = NULL;
static ThrdCtlBlk *nextTCB = NULL;
struct linkedlist *list;
static int threadCount = 0;
static int initialized = 0;
/* So far they're just empty pointers */

/*
 * Function for initializing the threads
 */
void init(){
    if(initialized==1){ return;} /* Safety */
    initialized = 1;
    threadCount = 1;
    
    /* Create a queue and a current Thread control block*/
    list = init_list();
    
    currentTCB = (ThrdCtlBlk *)malloc(sizeof(ThrdCtlBlk));
    currentTCB->context = (ucontext_t *)malloc(sizeof(ucontext_t));
    currentTCB->context->uc_stack.ss_size = ULT_MIN_STACK;
    currentTCB->context->uc_stack.ss_sp = (char *)malloc(ULT_MIN_STACK);
    getcontext(currentTCB->context);
    currentTCB->tid = 0;
}

/* Initializes the linkedlist */
struct linkedlist *init_list(){
    notFound = malloc(sizeof(ThrdCtlBlk));
    notFound->tid = -1;
    noneLeft = malloc(sizeof(ThrdCtlBlk));
    noneLeft->tid = -2;
    struct linkedlist *list = (struct linkedlist *)malloc(sizeof(struct linkedlist));
    list->size = 0;
    return list;
}

/* Adds a node to the linkedlist */
void append(struct linkedlist *list, ThrdCtlBlk *newNode)
{
    if(list->size == 0){ 
        list->headNode = (struct listNode *)malloc(sizeof(struct listNode));
        list->headNode->contents = newNode;
        list->size = 1;
    }
    else{ 
        struct listNode *node = (struct listNode *)malloc(sizeof(struct listNode));
        struct listNode *head = list->headNode; 
        node->contents = newNode;
        head->previous = node;
        node->previous = NULL;
        node->next = head;
        list->headNode = node;
        list->size = list->size + 1;
        return;
    }
}

/* Attempt to search list and return correct node */
/* or an error node if none can be found to match */
ThrdCtlBlk *removeNode(struct linkedlist *list, Tid tid){
    if(list->size == 0) { return noneLeft; } 
    
    struct listNode *node = list->headNode;
    ThrdCtlBlk *result = node->contents;
    int iFoundYou = 0;

    /*
    // If I Found You at the beginning
    // No fancy stuff
    */
    if(tid == result->tid){
        if(node->next != NULL)
        { node->next->previous = NULL; }
        list->headNode = node->next;
        free(node);
        list->size = list->size - 1 ;
        return result;
    }

    /* Otherwise */
    struct listNode *nextNode = node->next;
    Tid someTid;
    int i = 0;
    while(i < list->size){
        someTid = nextNode->contents->tid;
        if(tid == someTid){
            iFoundYou = 1;
            /* Tying up Loose Ends */
            if(i < list->size-1){
                node->next = nextNode->next;
                nextNode->next->previous = nextNode->previous;
            }
            result = nextNode->contents;
            free(nextNode);
            list->size = list->size - 1;
            return result;
        }
        if(i < list->size-2)
        { node = nextNode; nextNode = nextNode->next; }
    i++;
    }
    if(!(iFoundYou)) return notFound;
    return result;
}

/* Popin' LIFO style */
struct ThrdCtlBlk *pop(struct linkedlist *list){
    struct listNode *head = list->headNode;
    ThrdCtlBlk *result;
    if(list->size == 0) { return noneLeft; }
    result = head->contents;
    if(list->size == 1){
        free(head);
        list->size = 0;
        return result;
    }
    list->headNode = head->next;
    list->size = list->size - 1;
    free(head);
    return result;
}

/* Peeking all sneaky-like */
ThrdCtlBlk *peek(struct linkedlist *list){ return list->headNode->contents; }

/*==============*/
/* Free Methods */
/*==============*/

/* Free global error TCBs */
void freeGlobals(){ free(notFound); free(noneLeft); }

/* Free the entire list when we're done */
void freeList(struct linkedlist *list){
    freeGlobals();
    struct listNode *node = list->headNode;
    if(list->size <= 1){ free(node); free(list); return; }
    struct listNode *nextNode = node->next;
    int i = 0;
    while(i < list->size-1){
        free(node->contents->context->uc_stack.ss_sp);
        free(node->contents->context);
        free(node->contents);
        free(node);
        node = nextNode;
        nextNode = node->next;
        i++;
    }
    free(node);
    free(list);
    return;
}

/* Free TCB, stackPointer and context */
void freeTCB(ThrdCtlBlk *block){
    if(!block) return;
    if(block->context !=NULL){
        free(block->context->uc_stack.ss_sp);
        free(block->context);
    }
    free(block);
}

/* FREE ALL THE THINGS! */
void freeAll(){
    freeTCB(currentTCB);
    freeTCB(nextTCB);
    freeList(list);
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

Tid ULT_SWITCH(Tid wantTid){
    /* Check to see if initialized */
    volatile int didISwitch = 0;
    if(!initialized){ init(); /* Initialize thread and queue */ }

    /* if SELF, then just keep running */
    if(wantTid == ULT_SELF || wantTid == currentTCB->tid){
        nextTCB = currentTCB; return currentTCB->tid;
    }
    else if(wantTid == ULT_ANY){
        nextTCB = pop(list);
        if(nextTCB->tid == -2){ return ULT_NONE; }
    }
    else{
        nextTCB = removeNode(list, wantTid);
        if(nextTCB->tid < 0){ return ULT_INVALID; }
    }

    /* Perform the swap */
    if(!(didISwitch)){
        didISwitch = 1;
        /* Double Checking */
        if(!(wantTid == ULT_SELF || currentTCB->tid == wantTid)){
            append(list, currentTCB);
            currentTCB = nextTCB;
        }
        setcontext(currentTCB->context);
    }

    if(wantTid == ULT_ANY){ return currentTCB->tid; } 
    return wantTid;
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
    /* Check to see if wantTid is referencing an invalid tid */
    if((wantTid >= -2) && (wantTid < ULT_MAX_THREADS)){
        /* Perform the swap operation */
        return ULT_SWITCH(wantTid);
    }
    return ULT_INVALID;
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





