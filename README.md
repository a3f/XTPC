# Cross-Thread Procedure Call

Header-only library for recording a function call and running it in the context of an another thread, sharing the address space. Employs GNU [nested functions] and [`__builtin_apply`] under the hood and thus **only works with `gcc`**.

## Example
```c
/* scroll down or check the example-*s for more */
#include "xtpc.h"

enum event { CALL };
extern void blocking_send_to_eventloop(enum event type, void *data);
extern enum event poll_for_event;

int notify(struct xtpc *xtpc) {
    blocking_send_to_eventloop(CALL, xtpc);
    return 0;
}

static __thread const char *thread;
const char *getthread() { return thread; }

void dispatcher(void) {
    thread = __func__;

    #define XTPC_NOTIFY notify
    XTPC(puts)("This will be executed by the other thread when it comes around to it!")

    // Don't believe me? see this!

    const char *ret;

    ret = getthread();
    printf("getthread was run in %s\n", ret); // prints dispatcher
    
    ret = XTPC(getthread)();
    printf("getthread was run in %s\n", ret); // prints eventloop
}

void eventloop(void) {
    thread = __func__;

    for(;;) {
        switch(poll_for_events(&event)) {
            case CALL:
                struct xtpc *xtpc = event->payload;
                XTPC_SERVE(xtpc);
            break;
            
            /* handle all kinds of events */
        }
    }
}
```
## FAQ

#### ▪ Isn't that a Remote Procedure Call?
No, because the function is called in the *same* address space.

#### ▪ Isn't that just a plain old function call?
No, because the function is called in a *different* thread.

#### ▪ Isn't that just creating a new thread?
No, because the function is called in an *existing* thread.

#### ▪ Isn't that just calling SuspendThread and overriding the instruction pointer?
No, because the function is called *co-operatively*.

#### ▪ Isn't that just a workerpool with one thread?
No, because the function is called *synchronously*.

#### ▪ What good is it then?

Finding functions while reverse engineering a game is easy enough, the problem is calling those functions. They are often not thread-safe and thus calls need to be synchronized. Normally, this would mean synchronizing with the game's event loop or similar and writing special wrapper functions for **every** function you would like to call.

With XTPC, you don't need to write any wrappers. Enclosing the function name in `XTPC()` records the function call and sends it to the event loop. After it's processed, the function returns and you can continue business as usual.

This allows for *very* succint code, especially in [hooks]:

```c
#define XTPC_NOTIFY post_to_eventloop
int game_guiUpdateHealth_hook(int new_health) {
    if (new_health < 100) {
       // game_healWithPotion(1);    // not thread-safe
       XTPC(game_healWithPotion)(1); // but this is
    }

    return game_updateHealth_orig(int new_health);
}
```

[hooks]: https://github.com/a3f/ia32hook
[nested functions]: https://gcc.gnu.org/onlinedocs/gcc/Nested-Functions.html
[`__builtin_apply`]: https://gcc.gnu.org/onlinedocs/gcc/Constructing-Calls.html
