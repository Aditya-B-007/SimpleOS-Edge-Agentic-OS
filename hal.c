#include "hal.h"

// External architecture declarations moved from kernel.c
extern void arch_enable_interrupts(void);
extern void arch_disable_interrupts(void);
extern void arch_halt_cpu(void);
extern void arch_context_switch(uint32_t** old_sp, uint32_t* new_sp);

static void hal_enable_interrupts(void) {
    arch_enable_interrupts();
}

static void hal_disable_interrupts(void) {
    arch_disable_interrupts();
}

static void hal_halt_cpu(void) {
    arch_halt_cpu();
}

static void hal_context_switch(uint32_t** old_sp, uint32_t* new_sp) {
    arch_context_switch(old_sp, new_sp);
}

const HAL_Interface HAL_Impl = {
    .enable_interrupts = hal_enable_interrupts,
    .disable_interrupts = hal_disable_interrupts,
    .halt_cpu = hal_halt_cpu,
    .context_switch = hal_context_switch
};