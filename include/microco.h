#pragma once

#include <stddef.h>
#include <stdint.h>

/* Coroutine function type.
   All coroutine functions must match this signature.
*/
typedef void (*co_func)(void);

typedef struct co_t {
    uint32_t    *sp;      /* saved stack pointer */
    co_func      fn;      /* entry function */
    int          done;    /* finished flag */
    struct co_t *next; /* linked list of coroutines */
    uint32_t     sleep_until; /* sleep until timestamp */
} co_t;

/* Initialize a coroutine with a user-provided stack buffer.
   The stack buffer must be large enough to hold the coroutine's stack.
   The stack must be 8-byte aligned.
*/
void co_init(co_t *co, void *stack_mem, size_t stack_bytes,
                           co_func fn);

/* Returns control to the main context. When the coroutine is resumed is like
   if this function simply returned.

   Must be called from within a coroutine initialized with co_init and resumed
   with co_resume. That means, do not call the coroutine function directly.
*/
void co_yield(void);

/* Start or resume a coroutine.

   Must be called from the main context. Do not call from
   an interrupt handler.
*/
void co_resume(co_t *co);

/* Sleep for a specified duration.
   Must be called from within a coroutine.
*/
void co_sleep(uint32_t ms);

/* Call from an infinite loop in main context. 
   This is required for features like sleep.
*/
void co_loop(void);

/* Get the currently running coroutine. */
co_t * co_current(void);
