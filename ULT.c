#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */
#include <ucontext.h>
#include "ULT.h"

Tid firstCall = 0; 
linkedlist *list;
// Hotel available tids: (0 = available, 1 = occupied)
int available_Tids[ULT_MAX_THREADS]; 

// Function for initializing the linkedList
void listInit() 
{
  initialize_list();
  available_Tids[0] = 1;
  list->currentTCB->tid = 0;
  firstCall++;
}

void stub(void (*root)(void *), void *arg)
{
  Tid ret;
  (*root)(arg); // call root function
  ret = ULT_DestroyThread(ULT_SELF);
  assert(ret == ULT_NONE); // We should only get here if we are the last thread.
  // Free all memory for the linkedlist before final termination. FREE ALL THE THINGS!
  free(list->currentTCB->stack_orig); 
  free(list->currentTCB);
  free(list);
  exit(0); // all threads are done, so process should exit
}

Tid ULT_CreateThread(void (*fn)(void *), void *parg)
{
  // When we find an available tid, we'll set the new thread's tid to this
  int setTid;

  // First time call, just in case the list hasn't been initialized
  if(!firstCall){ listInit(); }
  assassinate();
  ThrdCtlBlk *newThread; // Create a new control block
  newThread = malloc(sizeof(ThrdCtlBlk)); // Allocate memory for it
  
  // If unable to malloc, then return ULT_NOMEMORY;
  if(!newThread){ return ULT_NOMEMORY; }
  
  //look for available tid (i.e. Check in the available_Tids array for an i such that available_Tids[i] == 0)
  int i = 0;
  while (i<ULT_MAX_THREADS)
  {
    if (available_Tids[i] == 0) // Got one.
    { setTid = i; break; }
    else{ i++; }
  }
  
  // Indicates that we cannot add in another thread 
  // because we've max out on the thread count.
  if(i == ULT_MAX_THREADS) 
  {
      free(newThread); // Fly away tcb! Be free!
      return ULT_NOMORE;
  }
  
  //tid no longer available
  available_Tids[i] = 1;
  
  // Save the current context into the new Thread's TCB
  int iGet = getcontext(&newThread->uc);
  assert(iGet == 0); // Did I get the context?
  newThread->marked = 0; 
  newThread->tid = setTid;
  newThread->sp = malloc(ULT_MIN_STACK); // Allocate memory for the stack.
  if (!newThread->sp)
  {
    free(newThread); // Since we can't allocate for the stack, we must free newThread
    return ULT_NOMEMORY;
  }
  
  /* Order of events */
  /*
  // 0riginal stack pointer used in order to free.
  // Set the stack pointer to the top of the stack.
  // Decrement stack pointer
  // Push the address of argument
  // Push the address of the function
  // We'll set the program counter to the address of stub
  // Set the stack pointer register to our stack pointer
  // Add Thread to the linkedlist
   */
  newThread->stack_orig = newThread->sp; 
  newThread->sp += (ULT_MIN_STACK/4); 
  (newThread->sp)--; 
  *(newThread->sp) = (unsigned int) parg; 
  (newThread->sp)--;
  *(newThread->sp) = (unsigned int) fn; 
  newThread->uc.uc_mcontext.gregs[REG_EIP] = (unsigned int)stub; 
  newThread->uc.uc_mcontext.gregs[REG_ESP] = (unsigned int)(--newThread->sp); 
  addTCB(list,newThread); 
  
  return newThread->tid;
}

Tid ULT_Yield(Tid wantTid)
{   
    // Indicates that the context save and loading is complete.
    volatile int doneThat = 0; 
    Tid retTid;
    
    // Double-checking list is initialized
    if(!firstCall)
    { listInit(); }
    assassinate(); //Any target found will be freed
    
    if(wantTid < -2 || wantTid >= ULT_MAX_THREADS) // Query out of bounds
    { return ULT_INVALID; }
    
    if(list == NULL) // There's nothing in here?! Looks like it
    { return ULT_NONE; }
    
    // Probably inquiring a non-assigned thread;
    if(available_Tids[wantTid] == 0 && (wantTid != ULT_SELF && wantTid != ULT_ANY)) 
    { return ULT_INVALID; }
    
    if(list->size == 0 && (wantTid != ULT_SELF))
    {
      // Safety net. Lucky catch
      if(list->currentTCB->tid == wantTid)
      { return list->currentTCB->tid; }
      else
      { return ULT_NONE; }
    }
        
    // If calling yourself
    if (wantTid == ULT_SELF || list->currentTCB->tid == wantTid)
    { return list->currentTCB->tid; }

    if(wantTid == ULT_ANY)
    {
      ThrdCtlBlk *tmp = list->head;
      if (!tmp) // Empty List
      { return ULT_NONE; }
      
      while(tmp) // Traverse through non-empty list to search for a non-target thread
      { 
	   if (tmp->marked)
	   { tmp = tmp->next; }
	   wantTid = tmp->tid;
	   break;
     }
     
     // All of the threads on queue are unkillable.
     if(!tmp) 
     { return ULT_NONE; }
   }
    
    ThrdCtlBlk *nextTCB = findTCB(list,wantTid); // Get the TCB to yield to in the linkedlist.

    if(!nextTCB)
    { return ULT_INVALID; }

    int ret = getcontext(&(list->currentTCB->uc)); // Save thread state (to TCB)
    assert(!ret);
    
    if(!doneThat)
    {
        doneThat = 1;
	ThrdCtlBlk *RT_To_List = list->currentTCB; // Does a swap with the currentTCB and the queried Thread
    
    
    if(!nextTCB) // Wasn't sure whether to delete this or not, but I think not because of context reasons.
    {
      return ULT_INVALID; // The thread with the queried tid is likely dead or just not there.
    }
    
    // Case 0: nextTCB is a lone element (head) in the linkedlist
    if(nextTCB->prev == NULL && nextTCB->next == NULL) 
    {
      list->currentTCB = nextTCB;
      list->head = RT_To_List;
      RT_To_List->prev = NULL;
      RT_To_List->next = NULL;
    }
    // Case 1: switch out the head    
    else if(nextTCB->prev == NULL && nextTCB->next != NULL)
    {
      addTCB(list,RT_To_List);
      list->size -= 1; // Equalize
      list->head = nextTCB->next;
      nextTCB->next->prev = NULL;
      nextTCB->next = NULL;
    }
    // Case 2: swtich out element in Middle of the List    
    else if(nextTCB->prev != NULL && nextTCB->next != NULL) 
    {
      addTCB(list,RT_To_List);
      list->size -= 1; // Equalize
      nextTCB->prev->next = nextTCB->next;
      nextTCB->next->prev = nextTCB->prev;
      nextTCB->prev = NULL;
      nextTCB->next = NULL;
    }
    // Case 3: switch out the tail    
    else if(nextTCB->prev != NULL && nextTCB->next == NULL)
    {
      nextTCB->prev->next = RT_To_List;
      RT_To_List->prev = nextTCB->prev;
      nextTCB->prev = NULL;
    }

    list->currentTCB = nextTCB; // Set the currentTCB to the queried thread.

    retTid = list->currentTCB->tid;
    setcontext(&(list->currentTCB->uc)); // Load its state (from TCB)
    // When we do this, if there is a fn in the thread, it runs NOW
    
    }
    return retTid;
}

Tid assassinate() // Finds a target thread and free's any data structures associated with it. Tid of killed target on success, ULT_NONE on failure.
{

	ThrdCtlBlk *iter = list->head;
	Tid retTid;
	while(iter) // Iterate through the linkedlist to find a target
	{
	  if (!iter->marked) // Keep going through the linkedlist until we find one or we reach the end
	  { iter = iter->next; }
	  else
	  { break; }
	}
	if (!iter) // No targets found. 
	{ return ULT_NONE; }
	else
	{
	  // Remove the target from the linkedlist in preparation for it's..."freeing".
	  // Pointer manipulations done in the following if/else if statements.
          // Case 0: Sole element in list.
	  if(iter->prev == NULL && iter->next == NULL)
	  { list->head = NULL; }
          // Case 1: It's the head!
	  else if(iter->prev == NULL && iter->next != NULL) 
	  {
	    list->head = iter->next;
	    list->head->prev = NULL;
	    iter->next = NULL;
	  }
          // Case 2: It's between the head and the tail
	  else if(iter->prev != NULL && iter->next != NULL) 
	  {
	    iter->next->prev = iter->prev;
	    iter->prev->next = iter->next;
	    iter->prev = NULL;
	    iter->next = NULL;
	  }
          // Case 3: It's a tail!
	  else if(iter->prev != NULL && iter->next == NULL) 
	  {
	    iter->prev->next = NULL;
	    iter->prev = NULL;
	  }
	  list->size -= 1; // Decrement size in response to removing the target.
	  retTid = iter->tid;
	  available_Tids[retTid] = 0;
	  free(iter->stack_orig);
	  free(iter);
	  return retTid;
	}
}
    
Tid ULT_DestroyThread(Tid tid) // Destroy policy for any. Kill the head of the linkedlist.
{
    
    if(!firstCall) // Initialize List like with Create and Yield.
    {
      listInit();
      return ULT_NONE; // Obviously, nothing is in the linkedlist.
    }
    if (list->size == 0 && tid == ULT_ANY) 
    { return ULT_NONE; }
    if(tid == ULT_SELF || list->currentTCB->tid == tid) 
    {    
      // Base Case: There are no threads in the ready list.
      // We won't kill the last remaining thread yet. We'll let the stub do it.
      if(list->size == 0) 
      { return ULT_NONE; } 
      // Default case: The size of the list is > 0
      else 
      {
	list->currentTCB->marked = 1; // I'm gonna get killed!
	ULT_Yield(ULT_ANY);
      }
    }
    
    Tid retTid;
    ThrdCtlBlk *iter = list->head;
    // Kill the target thread, then destroy the head thread like in the policy.
    if(tid == ULT_ANY)
    { retTid = assassinate(); }
    else{
	while (iter)
	{ // While temp is not null and target = 0, keep going until find target
	    if (iter->tid != tid){ iter = iter->next; }
	    else { break; }
	}
	// Either exited because found a target or because at end of list 
	
	if(iter == NULL)
	{ return ULT_INVALID; }
    }
    retTid = iter->tid;
    
    // Pull off the Thread from the linkedlist and free it and its associated data.
    if(iter->prev == NULL && iter->next == NULL) // Sole element in list.
    {
      list->head = NULL;
    }
    else if(iter->prev == NULL && iter->next != NULL) // It's the head!
    {
      list->head = iter->next;
      list->head->prev = NULL;
      iter->next = NULL;
    }
    else if(iter->prev != NULL && iter->next != NULL) // It's between the head and the tail
    {
      iter->next->prev = iter->prev;
      iter->prev->next = iter->next;
      iter->prev = NULL;
      iter->next = NULL;
    }
    else if(iter->prev != NULL && iter->next == NULL) // It's a tail!
    {
      iter->prev->next = NULL;
      iter->prev = NULL;
    }
    list->size -= 1; // Decrement List
    available_Tids[retTid] = 0; // Tid now vacant
    free(iter->stack_orig); // FREE STACK! No purchase necessary.
    free(iter); // Free the Thread
    return retTid;
}

void initialize_list()
{
  list = malloc(sizeof(linkedlist)); // Allocate memory for linkedlist
  list_init(list); // Initialize said linkedlist
}

/*
* Initialize data structures, returning pointer
* to the object.
*/
linkedlist *list_init(linkedlist *list){ // Initialize the linkedlist
    
    list->size = 0;
    list->head = NULL;
    
    ThrdCtlBlk *runner = malloc(sizeof(ThrdCtlBlk)); // Next 4 lines will set the current running thread
    int ret = getcontext(&(runner->uc));
    assert(!ret);
    list->currentTCB = runner;
    return list;
}

/*
* Add a new TCB to the end of the linkedlist.
*/
void addTCB(linkedlist *list, ThrdCtlBlk *TCBtoAdd)
{
    // Base Case.
    if (list->size == 0)
    { 
        list->head = TCBtoAdd;
        (list->size)++;
        return;
    }
    // Otherwise, append it at end of list
    // Go to end of list:
    
    ThrdCtlBlk *temp = list->head;
    while(temp->next != NULL)
    { temp = temp->next; }
    
    // Insert TCB at end
    temp->next = TCBtoAdd;
    TCBtoAdd->prev = temp;
    TCBtoAdd->next = NULL;
    
    (list->size)++; // Increment size
}
/*
* Find TCB based on given Thread ID and return TCB
*/
ThrdCtlBlk* findTCB(linkedlist *list, Tid target){
    if(list->currentTCB->tid == target) // Base case I, look at the running thread.
    { return list->currentTCB;  }
    if(list->size == 0) // Base Case II, list is empty
    {  return NULL; }
    ThrdCtlBlk *temp = list->head;
    ThrdCtlBlk *fail = NULL;
    // If first item in list
    if (temp->tid == target)
    { return temp; }
    temp = temp->next;
    // NOT first item in list
    while (temp != NULL){
        if (temp->tid == target)
        { 
            if(temp->marked){ return NULL; }
            return temp;
        }
        temp = temp->next;
    }
    
    // If can't find Tid in list, return 0
    return fail;
}