/* Save current SP into *from_sp, load *to_sp into SP, restore regs, return */
.syntax unified
.thumb

.global context_switch
.type context_switch,%function
/* void context_switch(uint32_t **from_sp, uint32_t **to_sp); */
context_switch:
    // save link register
    MOV R2, LR
    PUSH {R2}

    // Save R4–R7 using PUSH
    PUSH {R4-R7}

    // Manually save R8–R11 to current stack
    SUB SP, SP, #16         // Make space for R8–R11

    MOV R2, R8
    STR R2, [SP, #0]
    MOV R2, R9
    STR R2, [SP, #4]
    MOV R2, R10
    STR R2, [SP, #8]
    MOV R2, R11
    STR R2, [SP, #12]

    // Save current stack pointer to *from_sp
    MOV R2, SP      // Copy SP to R2
    STR R2, [R0]    // Store R2 to *from_sp

    // Load next stack pointer from *to_sp
    LDR R2, [R1]    // Load new SP value
    MOV SP, R2      // Update SP

    // Restore R8–R11 from next stack
    LDR R2, [SP, #0]
    MOV R8, R2
    LDR R2, [SP, #4]
    MOV R9, R2
    LDR R2, [SP, #8]
    MOV R10, R2
    LDR R2, [SP, #12]
    MOV R11, R2

    // Remove R8–R11 from stack
    ADD SP, SP, #16

    // Restore R4–R7 using POP
    POP {R4-R7}

    // Restore link register
    POP {R2}
    MOV LR, R2

    // Return to next task
    BX LR


.size context_switch, .-context_switch
