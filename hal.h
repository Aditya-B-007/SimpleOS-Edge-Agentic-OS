#ifndef SIMPLEOS_HAL_H
#define SIMPLEOS_HAL_H

#include <stdint.h>
#include <stdbool.h>
typedef struct {
    uint32_t cpu_architecture;      // 0=x86_64, 1=ARM64, etc.
    uint32_t cpu_id;                // Logical CPU ID
    uint32_t interrupt_controller_type; // Type of PIC/APIC/GIC
    uint32_t timer_type;            // Type of timer hardware
    uint64_t boot_info_address;     // Physical address of boot info (e.g. multiboot)
    uint64_t kernel_memory_base;    // Start of kernel memory
    uint64_t kernel_memory_limit;   // End of kernel memory
} HardwareContext;
typedef struct {
    uint32_t operation_code;        // Opcode for the transaction
    uint64_t input_address;         // Generic input pointer/addr
    uint64_t input_value;           // Generic numeric input
    uint64_t output_address;        // Result destination address
    uint32_t status_code;          
} HardwareTransaction;
typedef enum {
    HAL_OP_ENABLE_IRQ       = 1,
    HAL_OP_DISABLE_IRQ      = 2,
    HAL_OP_SET_TIMER        = 3,
    HAL_OP_READ_TIME        = 4,
    HAL_OP_MAP_PAGE         = 5,
    HAL_OP_UNMAP_PAGE       = 6,
    HAL_OP_ACK_IRQ          = 7,
    HAL_OP_INIT_HARDWARE    = 8
} kHalOperationCode;
typedef enum {
    HAL_STATUS_OK           = 0,
    HAL_STATUS_INVALID      = 1,
    HAL_STATUS_UNSUPPORTED  = 2,
    HAL_STATUS_FAILURE      = 3
} kHalStatusCode;

void hal_initialize_hardware(HardwareContext* context, HardwareTransaction* transaction);
void hal_enable_interrupts(HardwareContext* context, HardwareTransaction* transaction);
void hal_disable_interrupts(HardwareContext* context, HardwareTransaction* transaction);
void hal_configure_timer_tick(HardwareContext* context, HardwareTransaction* transaction);
void hal_read_monotonic_time(HardwareContext* context, HardwareTransaction* transaction);
void hal_acknowledge_interrupt(HardwareContext* context, HardwareTransaction* transaction);
void hal_map_memory_page(HardwareContext* context, HardwareTransaction* transaction);
void hal_unmap_memory_page(HardwareContext* context, HardwareTransaction* transaction);
#endif 
