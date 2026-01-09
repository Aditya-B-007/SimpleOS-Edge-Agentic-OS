#include "irq_manager.h"
#include "hal.h"

static volatile int g_critical_nesting = 0;

static void irq_enter_critical(void) {
    HAL_Impl.disable_interrupts();
    g_critical_nesting++;
}

static void irq_exit_critical(void) {
    if (g_critical_nesting > 0) {
        g_critical_nesting--;
        if (g_critical_nesting == 0) {
            HAL_Impl.enable_interrupts();
        }
    }
}

const InterruptManager_Interface IRQMgr_Impl = {
    .enter_critical = irq_enter_critical,
    .exit_critical = irq_exit_critical
};