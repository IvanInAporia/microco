#include <stdint.h>
#include <stddef.h>

#include "microco.h"

// Implemented in assembly
extern void context_switch(uint32_t **from_sp, uint32_t **to_sp);

/* Globals for simple single-scheduler setup */
static co_t    g_main_co;
static co_t   *g_current = &g_main_co;

/* Forward declarations */
static void co_entry(void);

/* Initialize a coroutine with a user-provided stack buffer */
void co_init(co_t *co, void *stack_mem, size_t stack_bytes,
                           co_func fn, void *arg)
{
    co->fn   = fn;
    co->arg  = arg;
    co->done = 0;

    /* Top of stack, 8-byte aligned */
    uint32_t *sp = (uint32_t *)(((uintptr_t)stack_mem + stack_bytes) & ~((uintptr_t)7));

    /* Prepare an initial "frame" so ctx_switch will pop into co_entry */
    *--sp = (uint32_t)co_entry;  /* LR for first return in target context */
    /* Reserve space for r4-r11 we pop in ctx_switch (9 regs including LR already set) */
    for (int i = 0; i < 8; ++i) *--sp = 0;

    co->sp = sp;
}

/* Yield back to main context */
void co_yield(void) {
    context_switch(&g_current->sp, &g_main_co.sp);
}

/* Resume a coroutine; returns when it yields or finishes */
void co_resume(co_t *co) {
    if (co->done) return;
    co_t *prev = g_current;
    g_current  = co;
    context_switch(&prev->sp, &co->sp);
    g_current  = prev;
}

/* Entry point that runs on the coroutine's own stack */
static void co_entry(void) {
    co_t *self = g_current;        /* set by co_resume before switching in */
    self->fn(self->arg);           /* run user code */
    self->done = 1;                /* mark finished */
    co_yield();                    /* return to main context */
}
