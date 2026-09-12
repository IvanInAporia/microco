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
 * - Implemented for ARM Cortex-M0/M0+ (the context switch is Thumb-1 assembly).
 *   Builds against any STM32 HAL: only HAL_GetTick() is used, and it is declared
 *   locally rather than pulled in from a family header.
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

/* sleep_until and status are polled by co_loop in the main context while being
   written from a coroutine or, for status, from an interrupt handler (co_resume
   called with a non-zero IPSR). They are volatile so the compiler cannot cache
   them across such a poll. sp is deliberately left unqualified: it is written by
   context_switch (assembly) and its address is passed as uint32_t**. */
typedef struct co_t {
    uint32_t             *sp;          /* saved stack pointer */
    co_func               fn;          /* entry function */
    struct co_t          *next;        /* linked list of coroutines */
    volatile uint32_t     sleep_until; /* sleep until timestamp */
    volatile co_status_t  status;      /* finished flag */
} co_t;

/* Initialize a coroutine with a user-provided stack buffer.
   The stack buffer must be large enough to hold the coroutine's stack.
   The stack must be 8-byte aligned.

   Declare the buffer as an array of uint32_t, not of uint8_t: co_init seeds the
   initial register frame by writing words through a uint32_t*, which is only
   well-defined if that is the buffer's effective type. Writing a uint8_t array
   that way breaks the aliasing rules the compiler assumes from -O2/-Os upwards.
   uint32_t alone still only guarantees 4-byte alignment, so keep the explicit
   8-byte alignment attribute.
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
