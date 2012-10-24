#ifndef _ULT_H_
#define _ULT_H_
#include <ucontext.h>
#include <stdlib.h>

typedef int Tid;
#define ULT_MAX_THREADS 1024
#define ULT_MIN_STACK 32768

/* List structures */
struct listNode{
    struct ThrdCtlBlk *contents;
    struct listNode *previous;
    struct listNode *next;
};
struct linkedlist{
    int size; 
    struct listNode * headNode;
};

/* Thread Control Block structures */
typedef struct ThrdCtlBlk{
    ucontext_t *context;     /* Pointer to ucontext object */
    Tid tid;                         /* hold the thread id */
} ThrdCtlBlk;

/*
 * Tids between 0 and ULT_MAX_THREADS-1 may
 * refer to specific threads and negative Tids
 * are used for error codes or control codes.
 * The first thread to run (before ULT_CreateThread
 * is called for the first time) must be Tid 0.
 */
static const Tid ULT_ANY = -1;
static const Tid ULT_SELF = -2;
static const Tid ULT_INVALID = -3;
static const Tid ULT_NONE = -4;
static const Tid ULT_NOMORE = -5;
static const Tid ULT_NOMEMORY = -6;
static const Tid ULT_FAILED = -7;

static inline int ULT_isOKRet(Tid ret){
  return (ret >= 0 ? 1 : 0);
}

void init();    /* initialization function */

Tid ULT_CreateThread(void (*fn)(void *), void *parg);
Tid ULT_SWITCH(Tid wantTid);
Tid ULT_Yield(Tid tid);
Tid ULT_DestroyThread(Tid tid);

/* List Functions */
struct linkedlist *init_list();
struct ThrdCtlBlk *peek(struct linkedlist *list);
void addNode(struct linkedlist *list, struct ThrdCtlBlk *newNode);
struct ThrdCtlBlk *removeNode(struct linkedlist *list, int tid);
struct ThrdCtlBlk *pop(struct linkedlist *list);
void freeGlobals();
void freeList(struct linkedlist *list);
void freeTCB(ThrdCtlBlk *block);
void freeAll();

#endif



