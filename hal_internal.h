#ifndef SIMPLEOS_HAL_INTERNAL_H
#define SIMPLEOS_HAL_INTERNAL_H

#include "hal.h"
#include <stdint.h>
typedef struct {
  uint64_t x[31]; // General purpose registers x0-x30
  uint64_t sp;    // Stack pointer
  uint64_t elr;   // Exception Link Register (PC)
  uint64_t spsr;  // Saved Program Status Register
} TrapFrame;
typedef struct {
  uint64_t ip;
  uint64_t sp;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
} ThreadFrame;
typedef struct {
  uint64_t acpi_rsdp_address;
  uint64_t mmio_base;
  uint64_t mmio_size;
} PlatformDescriptor;

void hal_internal_set_stack_pointer(uint64_t sp);
void hal_internal_set_cpu_mode(uint32_t mode);
uint64_t hal_internal_read_cpu_flags(void);
void hal_internal_write_cpu_flags(uint64_t flags);
void hal_internal_program_timer_hardware(uint64_t ticks);
uint64_t hal_internal_read_timer_hardware_counter(void);
void hal_internal_program_interrupt_controller(void);
void hal_internal_send_end_of_interrupt(uint32_t vector);
void hal_internal_write_page_table_entry(uint64_t virt, uint64_t phys,
                                         uint32_t flags);
void hal_internal_invalidate_tlb_entry(uint64_t virt);

#endif // SIMPLEOS_HAL_INTERNAL_H
