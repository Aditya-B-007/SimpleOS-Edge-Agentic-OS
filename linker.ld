/* SimpleOS Linker Script for Industrial Edge Devices */

MEMORY
{
    /* Define Flash and RAM regions (Adjust sizes based on target hardware) */
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
    RAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Entry Point */
ENTRY(Reset_Handler)

SECTIONS
{
    /* 1. Vector Table - Must be at the start of Flash */
    .isr_vector :
    {
        . = ALIGN(32);
        KEEP(*(.isr_vector))
        . = ALIGN(32);
    } > FLASH

    /* Define the start of the Kernel Image for CRC checks */
    __kernel_start = .;

    /* 2. Code Section */
    .text :
    {
        . = ALIGN(32);
        *(.text)
        *(.text*)
        *(.glue_7)         /* glue arm to thumb code */
        *(.glue_7t)        /* glue thumb to arm code */
        *(.eh_frame)
        . = ALIGN(32);
    } > FLASH

    /* 3. Read-only Data */
    .rodata :
    {
        . = ALIGN(32);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(32);
    } > FLASH

    /* 4. The .agents Section 
       Collects all AgentManifest structures. 
       KEEP() prevents the linker from optimizing these away. */
    .agents :
    {
        . = ALIGN(32);
        __start_agents = .;
        KEEP(*(.agents))
        __stop_agents = .;
        . = ALIGN(32);
    } > FLASH

    /* Define the end of the Kernel Image for CRC checks */
    __kernel_end = .;

    /* 5. Checksum Symbol
       This value is typically updated by a post-build CRC tool.
       The bootloader.c logic uses the address of this symbol as the value. */
    __kernel_checksum = 0xDEADBEEF; 

    /* 6. Initialized Data Section (VMA in RAM, LMA in FLASH) */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(32);
        _sdata = .;        /* Start of .data in RAM */
        *(.data)
        *(.data*)
        . = ALIGN(32);
        _edata = .;        /* End of .data in RAM */
    } > RAM AT > FLASH

    /* 7. Uninitialized Data Section (BSS) */
    .bss :
    {
        . = ALIGN(32);
        _sbss = .;         /* Used by startup.s */
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(32);
        _ebss = .;         /* Used by startup.s */
        __bss_end__ = _ebss;
    } > RAM

    /* 8. Stack and Heap Reservation */
    /* Reserve space for the main system stack */
    ._stack :
    {
        . = ALIGN(32);
        . = . + 0x2000;    /* 8KB Stack */
        _estack = .;       /* Top of stack used by startup.s */
    } > RAM

    /* Discard unnecessary sections */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }
}