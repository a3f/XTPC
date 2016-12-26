#include "xtpc.h"
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

struct eventloop_sync {
    enum { STATE_UNINIT, STATE_INIT, STATE_REQ, STATE_RESP } state;
    int ret;
    pthread_mutex_t lock;
    pthread_cond_t  cond;

    void *payload;
} *syn;

#define XTPC_NOTIFY notify
int notify(struct xtpc *xtpc) {
    pthread_mutex_lock(&syn->lock);

    syn->payload = xtpc;
    syn->state = STATE_REQ;

    pthread_cond_signal(&syn->cond);
    while (syn->state != STATE_RESP)
        pthread_cond_wait(&syn->cond, &syn->lock);

    pthread_mutex_unlock(&syn->lock);
    return 0;
}


static __thread const char * thread;
const char *getthread() { return thread; }

void *eventloop(void*);
int main(void) {
    thread = __func__;
    printf(":: Thread %s started up\n", getthread());

    // init condition variable that syncs the two threads
    syn = calloc(sizeof *syn, 1);
    pthread_mutex_init(&syn->lock, NULL);
    pthread_cond_init(&syn->cond, NULL);
    pthread_mutex_lock(&syn->lock);

    // spawn XTPC thread
    pthread_t pthread;
    pthread_create(&pthread, NULL, eventloop, syn);

    // wait till XTPC thread waits on condition variable
    while (syn->state == STATE_UNINIT)
        pthread_cond_wait(&syn->cond, &syn->lock);

    pthread_mutex_unlock(&syn->lock);

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

void *eventloop(void* syn_) {
    thread = __func__;
    printf("Thread %s started up\n", getthread());

    struct eventloop_sync *syn = syn_;
    pthread_mutex_lock(&syn->lock);
    syn->state = STATE_INIT;
    pthread_cond_signal(&syn->cond);

    for (;;) {
        while (syn->state != STATE_REQ)
            pthread_cond_wait(&syn->cond, &syn->lock);

        puts(":: eventloop calls");

        XTPC_SERVE((struct xtpc*)syn->payload);
        syn->state = STATE_RESP;
        pthread_cond_signal(&syn->cond);

        puts(":: eventloop called");

    }

    pthread_mutex_unlock(&syn->lock);

    return NULL;

}
