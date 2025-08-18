#include <stdint.h>
#include <stddef.h>

#include "stm32l0xx_hal.h"
#include "microco.h"

// Implemented in assembly
extern void context_switch(uint32_t **from_sp, uint32_t **to_sp);

/* Globals for simple single-scheduler setup */
static co_t    g_main_co;
static co_t   *g_current = &g_main_co;
static co_t   *g_list    = NULL;       // linked list of coroutines

/* Forward declarations */
static void co_entry(void);

/* Initialize a coroutine with a user-provided stack buffer */
void co_init(co_t *co, void *stack_mem, size_t stack_bytes,
                           co_func fn)
{
    co->fn   = fn;
    co->done = 0;
    co->next = g_list;
    g_list   = co;

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

void co_sleep(uint32_t ms) {
    // Sleep for a specified duration
    uint32_t start = HAL_GetTick();

    g_current->sleep_until = start + ms;

    // 0 will mean not sleeping, so change to 1 even though this will
    // give us one ms additional than requested. No problem since this
    // sleep is not designed to be exact
    if (g_current->sleep_until == 0)
    {
        g_current->sleep_until = 1;
    }
    co_yield();
}

void co_loop(void)
{
    co_t *p = g_list;

    uint32_t now = HAL_GetTick();
    while (p) {
        // If the coroutine is sleeping, check if it's time to wake it up
        if (p->sleep_until != 0) {
            if (now >= p->sleep_until) {
                p->sleep_until = 0; // Reset sleep until
                co_resume(p);
            }
        }
        
        p = p->next;
    }
}

co_t * co_current(void) {
    return g_current;
}

/* Entry point that runs on the coroutine's own stack */
static void co_entry(void) {
    co_t *self = g_current;        /* set by co_resume before switching in */
    self->fn();           /* run user code */
    self->done = 1;                /* mark finished */
    co_yield();                    /* return to main context */
}
