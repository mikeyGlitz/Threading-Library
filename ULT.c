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
static int nex = 0;
static int tidsFull = 0;
//static int tidCounter = 0; // Use instead of assignTid
int available_Tids[ULT_MAX_THREADS]; //ULT_MAX_THREADS vacant slots for Tids (0 = available, 1 = occupied)
static int destroy = 0;
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
    
    available_Tids[0] = 1;
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
        /* No FOR loops! */
        //for(int i=1; i < list->size; i++)
        int i=1;
        while(i < list->size)
        { head = head->next; i++; }
        head->next = node;
        node->previous = head;
        node->next = NULL;
        /*
        head->previous = node;
        node->previous = NULL;
        node->next = head;
        list->headNode = node;
        */
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
                /* Check if at end of list or not STUPID! */
                if(nextNode->next != NULL)
                { nextNode->next->previous = nextNode->previous; }
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

/* Popin' LIFO style FAIL! */
/* Popin' FIFO style! */
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
    head->next->previous = NULL;
    head->next = NULL;
    list->size = list->size - 1;
    free(head);
    head = NULL;
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
    /* Initialize Threading, will do nothing if already done */
    if(!initialized){ init(); }
    int setTid;

    /* Check to see if any threads available */
    if(threadCount >= ULT_MAX_THREADS)
    { return ULT_NOMORE; }
    
    /* Initialize new thread */
    ThrdCtlBlk *newThread = (ThrdCtlBlk *)malloc(sizeof(ThrdCtlBlk));
    newThread->context = (ucontext_t *)malloc(sizeof(ucontext_t));
    
     void * stack = (stack_t *)malloc(ULT_MIN_STACK * sizeof(int));
    if(!stack) { return ULT_NOMEMORY;}

    /*  To top, -4, Push Parg, down4, push FN, down4, set stack pointer */
    stack += ULT_MIN_STACK;
    stack -= 4;
    *(int *)stack = (unsigned int) parg;
    stack -= 4;
    *(int *)stack = (unsigned int) fn;
    stack -= 4;

    /* getcontext to store in newThread and open some space for stack pointer */
    getcontext(newThread->context);
    newThread->context->uc_stack.ss_sp = (char *)malloc(ULT_MIN_STACK);
    newThread->context->uc_stack.ss_size = ULT_MIN_STACK;

    /* MY VIRST ATTEMPT
    newThread->context->uc_stack.ss_sp = (unsigned int *) stack;
    newThread->sp = (unsigned int *) stack;
    newThread->context->uc_mcontext.gregs[REG_ESP] = (unsigned int)stack;
    // (void*)((unsigned int)new_stack + (ULT_MIN_STACK - (3 * sizeof(void*))));
    newThread->context->uc_mcontext.gregs[REG_EIP] = (unsigned int)stub;
     */
    
    /* MY SECUNDA ATTEMPT */
    newThread->sp = malloc(ULT_MIN_STACK); //allocate memory for the stack.
    newThread->sp += (ULT_MIN_STACK/4); //Set the stack pointer to the top of the stack
    (newThread->sp)--; //Decrement stack pointer
    *(newThread->sp) = (unsigned int) parg; //push the address of argument
    (newThread->sp)--;
    *(newThread->sp) = (unsigned int) fn; //push the address of the function
    newThread->context->uc_mcontext.gregs[REG_EIP] = (unsigned int)stub; //We'll set the program counter to the address of stub
    newThread->context->uc_mcontext.gregs[REG_ESP] = (unsigned int)(--newThread->sp); //Set the stack pointer register to our stack pointer
    
    /* MakeContext */
    /* DON'T USE */
    //makecontext(newThread->context, (void (*)(void))stub, 2, fn, parg);
    /* Assign Stack Pointer and EIP instead */
    /*
    newThread->context->uc_mcontext.gregs[REG_EDI] = (unsigned int)fn;
    newThread->context->uc_mcontext.gregs[REG_ESI] = (unsigned int)parg;
    */
        /* REFERENCE MATERIAL
   new_tcb->stack_orig = new_tcb->sp; //original stack pointer used in order to free.
  new_tcb->sp += (ULT_MIN_STACK/4); //Set the stack pointer to the top of the stack
  (new_tcb->sp)--; //Decrement stack pointer
  *(new_tcb->sp) = (unsigned int) parg; //push the address of argument
  (new_tcb->sp)--;
  *(new_tcb->sp) = (unsigned int) fn; //push the address of the function
  new_tcb->uc.uc_mcontext.gregs[REG_EIP] = (unsigned int)stub; //We'll set the program counter to the address of stub
  new_tcb->uc.uc_mcontext.gregs[REG_ESP] = (unsigned int)(--new_tcb->sp); //Set the stack pointer register to our stack pointer
  addTCB(rl,new_tcb); //Add Thread to the readyList
        */
    
    /* Assign new Tid */
    //newThread->tid = assignId();
    //if(newThread->tid < 0){ return ULT_NOMEMORY; }
    //if(tidCounter+1 >= ULT_MAX_THREADS) { return ULT_NOMEMORY; }
    //tidCounter++;
    //newThread->tid = tidCounter;
     int i = 0;
     while (i<ULT_MAX_THREADS)
     {
       if (available_Tids[i] == 0) // Got One!
       { setTid = i; break; }
       else{ i++; }
     }
    if(i == ULT_MAX_THREADS) // Stupid, why waste time making things you can't keep?
    {
      free(newThread); // Fly free new thread. You can't live here
      return ULT_NOMORE;
    }
  
    // Lock that Tid Door, and set it
    available_Tids[i] = 1;
    newThread->tid = setTid;
    
    //if(newThread->tid == -1) { return ULT_NOMEMORY; }
    append(list,newThread);
    threadCount +=1;
    return newThread->tid;
}

Tid ULT_SWITCH(Tid wantTid){
    /* Check to see if initialized */
    volatile int didISwitch = 0;
    if(!initialized){ init(); /* Initialize thread and queue */ }

    /* if SELF, then just keep running */
    if(wantTid == ULT_SELF || wantTid == currentTCB->tid){
        //nextTCB = currentTCB;
        return currentTCB->tid;
    }
    else if(wantTid == ULT_ANY){
        nextTCB = pop(list);
        if(nextTCB->tid == -2){ return ULT_NONE; }
    }
    else{
        nextTCB = removeNode(list, wantTid);
        if(nextTCB->tid < 0){ return ULT_INVALID; }
    }
    int ret = getcontext(currentTCB->context); //save thread state (to TCB)
    assert(!ret);
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
  /* Double Checking  */
  if(!initialized){ init(); return ULT_NONE; }
  if (list->size == 0 && tid == ULT_ANY){ return ULT_NONE; }
  
  ThrdCtlBlk *destroyblk;
  int destid;
  /* Tid comptid; */

  // If self is requested, yield to next thread, and mark self for destruction
  if (tid == ULT_SELF || tid == currentTCB->tid){
    destroy = 1;
    /* comptid = currentTCB->tid; */
    if(list->size == 0){ return ULT_NONE; }
    threadCount--;   
    Tid yeld = ULT_Yield(ULT_ANY);
    //if none left, exit program
    if (yeld == ULT_NONE) {
        freeAll();
        exit(0);
     }
    available_Tids[yeld] = 0; // Tid now vacant
  }

  // If request to destroy any, pop the list and destroy resulting thread
  else if (tid == ULT_ANY){
    destroyblk = pop(list);
    if (destroyblk->tid == -2){ return ULT_NONE; }
    threadCount--;
    available_Tids[destroyblk->tid] = 0; // Tid now vacant
    destid = destroyblk->tid;
    freeTCB(destroyblk);
    return destid;
  }

  // Otherwise, remove requested node
  else {
    destroyblk = removeNode(list,tid);
    destid = destroyblk->tid;
    threadCount--;
    available_Tids[destroyblk->tid] = 0; // Tid now vacant
    if (destid == -1 || destid == -2){ return ULT_INVALID; }
    freeTCB(destroyblk);
    return tid;
   }

  return ULT_FAILED;
}

void stubFn(void (*fn)(void *), void *arg)
{
        fn(arg);
        ULT_DestroyThread(ULT_SELF);
}

/*
 * Assign a Tid
 */
Tid assignId()
{
  // Decide if we should implement at 0 or 1
  if (list->size == 0){
    if (currentTCB->tid !=0) nex = 0;
    else nex = 1;}

  if (nex == ULT_MAX_THREADS) tidsFull = 1;

  // If queue is now full, simply increment the value to be assigned
  // otherwise, search for an unused id
  if (!tidsFull){
    nex++;
    return nex-1;
  }

  else {
   nex = 0;
   int i;
   for (i = 0; i< ULT_MAX_THREADS; i++){
     if (!threadForTid(list,nex) || nex != currentTCB->tid) return nex;
   }
  }
  return -1;

}

int threadForTid (struct linkedlist *list, Tid tid)
{
	if (list->size == 0) { return 0; }
	struct listNode *currentThread = list->headNode;
	while (currentThread)
	{
		if (currentThread->contents->tid == tid)
		{ return 1; }
		currentThread = currentThread->next;
	}
	return 0;
}

void stub(void (*root)(void *), void *arg)
{
    // thread starts here
    Tid ret;
    root(arg); // call root function
    ret = ULT_DestroyThread(ULT_SELF);
    assert(ret == ULT_NONE); // we should only get here if we are the last thread.
    exit(0); // all threads are done, so process should exit
}

