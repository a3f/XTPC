#ifndef XTPC_H_
#define XTPC_H_

#include <stddef.h>
#include <stdint.h>

struct xtpc {
    void (*func)();
    void *args;
    size_t size;

    void *ret;
};

#ifdef XTPC_NOP

#define XTPC_SERVE(xtpc) 0
#define NO_XTPC(fun) (fun)
#define XTPC(fun) (fun)

#else /* XTPC_NOP */

#define XTPC_SERVE(xtpc) ({\
            (xtpc)->ret = (__builtin_apply(\
                    (void*)((xtpc)->func),\
                    (xtpc)->args,\
                    (xtpc)->size)\
                );\
        })
#define NO_XTPC(fun) (fun)
#define XTPC_RETURN(ret) __builtin_return((ret))
#define XTPC(fun) \
    ({ \
     intptr_t _fn_ () { \
        struct xtpc xtpc = { (void(*)())(fun), __builtin_apply_args(), 127, 0 };\
        XTPC_NOTIFY(&xtpc);\
        (void)XTPC_RETURN(xtpc.ret);\
        return 0;\
     } \
     _fn_; \
     })

#endif /* XTPC_NOP */

#endif
