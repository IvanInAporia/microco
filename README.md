# Minimal Coroutine Library for STM32L0

## Overview

This library provides a minimal coroutine implementation suitable for embedded systems. The main goal is to have stackful cooperative coroutines using the least amount of memory possible.

To save memory, only the following features are supported:
- Cooperative coroutines via yield and resume
- Coroutine sleep

For now, it has these limitations:
- Implemented for STM32L0. But might be easy to modify for other ARM processors.
- Cannot resume from an interrupt.
- Cannot directly pass parameters through yield nor resume.

## Usage Notes

- **Do not call coroutine functions directly**; use `co_resume` to start/resume.
- **All coroutine stacks must be properly aligned and sized** (8-byte alignment required).
- **Call `co_loop` regularly** to handle sleeping coroutines. This is usually done in an infinite loop in the main context.

## API Reference

### Coroutine Function Type

```c
typedef void (*co_func)(void);
```
All coroutine functions must match this signature.

### Functions

#### Initialize a Coroutine

```c
void co_init(co_t *co, void *stack_mem, size_t stack_bytes, co_func fn);
```
Initializes a coroutine with a user-provided stack buffer. The stack buffer must be large enough and 8-byte aligned.

#### Yield

```c
void co_yield(void);
```
Returns control to the main context. Must be called from within a coroutine initialized with `co_init` and resumed with `co_resume`.

#### Resume

```c
void co_resume(co_t *co);
```
Start or resume a coroutine. Must be called from the main context. Do not call from an interrupt handler.

#### Sleep

```c
void co_sleep(uint32_t ms);
```
Sleep for a specified duration. Must be called from within a coroutine.

#### Scheduler Loop

```c
void co_loop(void);
```
Call from an infinite loop in the main context. Required for the sleep feature.

#### Get Current Coroutine

```c
co_t * co_current(void);
```
Get the currently running coroutine.

## Example

```c
static co_t co_worker;
static uint8_t stack_worker[128] __attribute__((aligned(8)));
static bool tx_done = false;

static void worker() {
    while (1) {
        HAL_UART_Transmit_IT(&huart2, "Worker: running\n", 16);

        // Yield to wait for the transmission to be done
        co_yield();

        // Sleep for 500 ms before next iteration
        co_sleep(500);
    }
}

// Callback when the transmission is done
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    tx_done = true;
}

int main(void) {
    // Initialize the coroutine with its stack and entry function
    co_init(&co_worker, stack_worker, sizeof(stack_worker), worker);

    // Start the coroutine
    co_resume(&co_worker);

    while (1) {
        // Resume is done here since cannot call co_resume from an interrupt
        if (tx_done)
        {
            tx_done = false;
            co_resume(&co_worker);
        }

        // Handle sleeping coroutines (required for co_sleep)
        co_loop();
    }
}
```

Find a more complex example in the microco_example folder, which is a full STM32 IDE project.

## License

This software is released under the MIT License.

```
MIT License

Copyright (c) 2024 [Your Name or Organization]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
