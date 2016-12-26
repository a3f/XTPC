#include "xtpc.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define WINVER 0x0600  
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600  
#include <windows.h>
#include <synchapi.h>

struct eventloop_sync {
    enum { STATE_UNINIT, STATE_INIT, STATE_REQ, STATE_RESP } state;
    int ret;
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE cond;

    void *payload;
} *syn;

#define XTPC_NOTIFY notify
int notify(struct xtpc *xtpc) {
    EnterCriticalSection(&syn->lock);

    syn->payload = xtpc;
    syn->state = STATE_REQ;

    WakeConditionVariable(&syn->cond);
    while (syn->state != STATE_RESP)
        SleepConditionVariableCS(&syn->cond, &syn->lock, INFINITE);

    LeaveCriticalSection(&syn->lock);
    return 0;
}


static __thread const char * thread;
const char *getthread() { return thread; }

DWORD WINAPI eventloop(void *syn_);
int main(void) {
    thread = __func__;
    printf(":: Thread %s started up\n", getthread());

    // init condition variable that syncs the two threads
    syn = calloc(sizeof *syn, 1);
    InitializeCriticalSection(&syn->lock);
    InitializeConditionVariable(&syn->cond);
    EnterCriticalSection(&syn->lock);

    // spawn XTPC thread
    CreateThread( 
            NULL,      // default security attributes
            0,         // use default stack size  
            eventloop, // thread function name
            syn,       // argument to thread function 
            0,         // use default creation flags 
            NULL
            );     // returns the thread identifier 

    // wait till XTPC thread waits on condition variable
    while (syn->state == STATE_UNINIT)
        SleepConditionVariableCS(&syn->cond, &syn->lock, INFINITE);

    LeaveCriticalSection(&syn->lock);

    // run puts in the context of the XTPC thread
    int len = XTPC(puts)("Hello World");
    printf("::Remote printf returned %d\n\n", len);

    const char *ret;

    ret = getthread();
    printf("getthread was run in %s\n", ret); // prints dispatcher

    ret = (void*)XTPC(getthread)();
    printf("getthread was run in %s\n", ret); // prints eventloop

    return 0;
}
DWORD WINAPI eventloop(void *syn_) {
    thread = __func__;
    printf("Thread %s started up\n", getthread());

    struct eventloop_sync *syn = syn_;
    EnterCriticalSection(&syn->lock);
    syn->state = STATE_INIT;
    WakeConditionVariable(&syn->cond);

    for (;;) {
        while (syn->state != STATE_REQ)
            SleepConditionVariableCS(&syn->cond, &syn->lock, INFINITE);

        puts(":: eventloop calls");

        XTPC_SERVE((struct xtpc*)syn->payload);
        syn->state = STATE_RESP;
        WakeConditionVariable(&syn->cond);

        puts(":: eventloop called");

    }

    LeaveCriticalSection(&syn->lock);

    return 0;
}
