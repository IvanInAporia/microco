/*
 * microco.h - Minimal coroutine library interface
 *
 * This header defines the API for a minimal coroutine library suitable for
 * embedded systems. The main goal is to to have a stackful cooperative 
 * coroutines using the least amount of memory possible.
 *
 * For saving memory only the following features are supported:
 * - Cooperative coroutine via yield and resume
 * - Coroutine sleep
 * 
 * For now it has these limitations:
 * - Implemented for STM23L0. But might be easy to modify for other arm processors.
 * - Cannot start a coroutine from another coroutine
 * - Cannot directly pass parameters through yield nor resume.
 *
 * Usage Notes:
 *   - Do not call coroutine functions directly; use co_resume to start/resume.
 *   - All coroutine stacks must be properly aligned and sized.
 *   - co_loop must be called regularly to handle sleep and resume from interrupt.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

/* Coroutine function type.
   All coroutine functions must match this signature.
*/
typedef void (*co_func)(void);

typedef enum {
    CO_STATUS_IDLE,     // Coroutine just created
    CO_STATUS_MAIN,     // Not a real status, represents the main context which cannot yield nor be resumed
    CO_STATUS_READY,    // Ready to run, will be resumed on next call to co_loop
    CO_STATUS_RUNNING,  // Currently running
    CO_STATUS_WAITING,  // Coroutine yielded. Waiting to be resumed.
    CO_STATUS_SLEEPING, // Sleeping
    CO_STATUS_FINISHED  // Coroutine function returned. Cannot be resumed anymore.
} co_status_t;

typedef struct co_t {
    uint32_t    *sp;          /* saved stack pointer */
    co_func      fn;          /* entry function */
    struct co_t *next;        /* linked list of coroutines */
    uint32_t     sleep_until; /* sleep until timestamp */
    co_status_t  status;      /* finished flag */
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

   Must be called from the main context or
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
