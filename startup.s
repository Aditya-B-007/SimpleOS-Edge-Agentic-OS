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
Reset_Handler:
    ldr r0, =_estack
    mov sp, r0
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
```

### 2. Bootloader Header (`bootloader.h`)
Defines the entry point for the bootloader.

```diff
