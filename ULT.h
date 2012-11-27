#ifndef _ULT_H_
#define _ULT_H_
#include <ucontext.h>

typedef int Tid;
#define ULT_MAX_THREADS 1024
#define ULT_MIN_STACK 32768

typedef struct ThrdCtlBlk
{
    Tid tid;
    ucontext_t uc; //context for the threads
    unsigned int *sp; //Allocate memory and point to the stack
    unsigned int *stack_orig; //this points to the top of the original stack so when free, free all mem
    int marked; // Bool(?) for assassinations.
    struct ThrdCtlBlk *prev;
    struct ThrdCtlBlk *next;   
} ThrdCtlBlk;

typedef struct linkedlistStruct {
   int size; //size of linkedlist
   ThrdCtlBlk *head; //The first element in the linkedlist
   ThrdCtlBlk *currentTCB; //The currently running thread
} linkedlist;

/*
 * Tids between 0 and ULT_MAX_THREADS-1 may
 * refer to specific threads and negative Tids
 * are used for error codes or control codes.
 * The first thread to run (before ULT_CreateThread
 * is called for the first time) must be Tid 0.
 */
static const Tid ULT_ANY = -1; //Used By: Destroy and Yield
static const Tid ULT_SELF = -2; //Used By: Destroy and Yield
static const Tid ULT_INVALID = -3; //Returned By: Destroy and Yield
static const Tid ULT_NONE = -4; //Returned By: Destroy and Yield
static const Tid ULT_NOMORE = -5;  //Returned By: Create
static const Tid ULT_NOMEMORY = -6; //Returned By: Create

static inline int ULT_isOKRet(Tid ret){
  return (ret >= 0 ? 1 : 0);
}

void listInit();
Tid assassinate();
void initStack(unsigned int *s);
void stub(void (*fn)(void *), void *arg);
Tid ULT_CreateThread(void (*fn)(void *), void *parg);
Tid ULT_Yield(Tid tid);
Tid ULT_DestroyThread(Tid tid);
void initialize_list();

linkedlist *list_init(linkedlist *list);
void addTCB(linkedlist *list, ThrdCtlBlk *TCBtoAdd);
ThrdCtlBlk* findTCB(linkedlist *list, Tid target);


#endif
