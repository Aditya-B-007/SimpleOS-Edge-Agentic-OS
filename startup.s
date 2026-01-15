.syntax unified
.cpu cortex-m4
.thumb
.global Reset_Handler
.global _start
.word _estack       /* End of Stack (Initial SP) */
.word _sbss         /* Start of BSS */
.word _ebss         /* End of BSS */
.section .isr_vector, "a"
.type g_pfnVectors, %object
g_pfnVectors:
    .word _estack
    .word Reset_Handler

.section .text.reset
.type Reset_Handler, %function
_start:
Reset_Handler:
    ldr r0, =_estack
    mov sp, r0

    /* Initialize Kernel Stack */
    ldr r0, =_sstack
    ldr r1, =_estack
    ldr r2, =0xDEADBEEF
stack_loop:
    cmp r0, r1
    bge stack_done
    str r2, [r0], #4
    b stack_loop
stack_done:

    ldr r0, =_sbss
    ldr r1, =_ebss
    mov r2, #0

bss_loop:
    cmp r0, r1
    bge bss_done
    str r2, [r0], #4
    b bss_loop

bss_done:
    /* 3. Jump to Bootloader Main */
    bl bootloader_main

    /* Should never return */
hang:
    b hang

/* --- HAL Architecture Implementations --- */

.global arch_enable_interrupts
.type arch_enable_interrupts, %function
arch_enable_interrupts:
    cpsie i
    bx lr

.global arch_disable_interrupts
.type arch_disable_interrupts, %function
arch_disable_interrupts:
    cpsid i
    bx lr

.global arch_halt_cpu
.type arch_halt_cpu, %function
arch_halt_cpu:
    wfi
    bx lr

.global arch_init_mpu
.type arch_init_mpu, %function
arch_init_mpu:
    bx lr

.global arch_context_switch
.type arch_context_switch, %function
arch_context_switch:
    push {r4-r11, lr}
    str sp, [r0]
    mov sp, r1
    pop {r4-r11, pc}
```

### 2. Bootloader Header (`bootloader.h`)
Defines the entry point for the bootloader.

```diff
