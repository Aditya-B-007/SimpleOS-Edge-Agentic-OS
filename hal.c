#include "hal.h"
#include "hal_internal.h"
#include <stddef.h>

static HardwareContext g_hw_context;

/**
 * @brief Validates HAL function parameters and sets error status if invalid.
 * @param context Pointer to HardwareContext.
 * @param transaction Pointer to HardwareTransaction.
 * @return 0 if valid, 1 if invalid (status_code set to HAL_STATUS_INVALID).
 */
static int validate_hal_params(HardwareContext *context,
                               HardwareTransaction *transaction) {
  if (!context || !transaction) {
    if (transaction)
      transaction->status_code = HAL_STATUS_INVALID;
    return 1; // Invalid
  }
  return 0; // Valid
}

void hal_initialize_hardware(HardwareContext *context,
                             HardwareTransaction *transaction) {
  if (validate_hal_params(context, transaction))
    return;
  g_hw_context = *context;

  hal_internal_program_interrupt_controller();

  transaction->status_code = HAL_STATUS_OK;
}

void hal_enable_interrupts(HardwareContext *context,
                           HardwareTransaction *transaction) {
  if (validate_hal_params(context, transaction))
    return;

  transaction->status_code = HAL_STATUS_OK;
}

void hal_disable_interrupts(HardwareContext *context,
                            HardwareTransaction *transaction) {
  if (validate_hal_params(context, transaction))
    return;

  transaction->status_code = HAL_STATUS_OK;
}

void hal_configure_timer_tick(HardwareContext *context,
                              HardwareTransaction *transaction) {
  if (validate_hal_params(context, transaction))
    return;

  uint64_t freq = transaction->input_value;
  hal_internal_program_timer_hardware(freq);

  transaction->status_code = HAL_STATUS_OK;
}

void hal_read_monotonic_time(HardwareContext *context,
                             HardwareTransaction *transaction) {
  if (validate_hal_params(context, transaction))
    return;

  uint64_t time = hal_internal_read_timer_hardware_counter();

  if (transaction->output_address != 0) {
    // Validate output address is within kernel memory bounds
    if (transaction->output_address >= context->kernel_memory_base &&
        transaction->output_address + sizeof(uint64_t) <=
            context->kernel_memory_limit) {
      *(uint64_t *)(transaction->output_address) = time;
    } else {
      transaction->status_code = HAL_STATUS_INVALID;
      return;
    }
  }

  transaction->status_code = HAL_STATUS_OK;
}

void hal_save_cpu_context(HardwareContext *context,
                          HardwareTransaction *transaction) {
  if (validate_hal_params(context, transaction))
    return;

  TrapFrame *tf = (TrapFrame *)(transaction->output_address);
  if (tf) {
    tf->elr = 0xDEADBEEF; // Dummy
  }

  transaction->status_code = HAL_STATUS_OK;
}

void hal_restore_cpu_context(HardwareContext *context,
                             HardwareTransaction *transaction) {
  if (validate_hal_params(context, transaction))
    return;

  TrapFrame *tf = (TrapFrame *)(transaction->input_address);
  if (tf) {
    hal_internal_set_stack_pointer(tf->sp);
  }

  transaction->status_code = HAL_STATUS_OK;
}

void hal_acknowledge_interrupt(HardwareContext *context,
                               HardwareTransaction *transaction) {
  if (validate_hal_params(context, transaction))
    return;

  uint32_t vector = (uint32_t)transaction->input_value;
  hal_internal_send_end_of_interrupt(vector);

  transaction->status_code = HAL_STATUS_OK;
}

void hal_map_memory_page(HardwareContext *context,
                         HardwareTransaction *transaction) {
  if (validate_hal_params(context, transaction))
    return;

  hal_internal_write_page_table_entry(transaction->output_address,
                                      transaction->input_address,
                                      (uint32_t)transaction->input_value);

  transaction->status_code = HAL_STATUS_OK;
}

void hal_unmap_memory_page(HardwareContext *context,
                           HardwareTransaction *transaction) {
  if (validate_hal_params(context, transaction))
    return;

  hal_internal_invalidate_tlb_entry(transaction->input_address);

  transaction->status_code = HAL_STATUS_OK;
}

void hal_internal_set_stack_pointer(uint64_t sp) { (void)sp; }

void hal_internal_set_cpu_mode(uint32_t mode) { (void)mode; }

uint64_t hal_internal_read_cpu_flags(void) {
  uint64_t flags = 0;
  return flags;
}

void hal_internal_write_cpu_flags(uint64_t flags) { (void)flags; }

void hal_internal_program_timer_hardware(uint64_t ticks) {
#ifdef __aarch64__
  // Write Time Value
  __asm__ volatile("msr cntp_tval_el0, %0" : : "r"(ticks));
  // Enable EL0 physical timer (bit 0 = enable, bit 1 = imask)
  uint64_t ctl = 1;
  __asm__ volatile("msr cntp_ctl_el0, %0" : : "r"(ctl));
#else
  (void)ticks;
#endif
}

uint64_t hal_internal_read_timer_hardware_counter(void) {
  uint64_t cnt = 0;
#ifdef __aarch64__
  __asm__ volatile("mrs %0, cntpct_el0" : "=r"(cnt));
#else
  static uint64_t host_counter = 0;
  cnt = host_counter++;
#endif
  return cnt;
}

void hal_internal_program_interrupt_controller(void) {
  // Skeleton for GIC initialization
  // In a real implementation, we would use g_hw_context.mmio_base
  // to configure the GIC Distributor and CPU Interface.
}

void hal_internal_send_end_of_interrupt(uint32_t vector) {
  // Skeleton for GIC EOI
  // Would write 'vector' to the GIC CPU Interface EOI register (GICC_EOIR)
}

void hal_internal_write_page_table_entry(uint64_t virt, uint64_t phys,
                                         uint32_t flags) {
  // Stub
}

void hal_internal_invalidate_tlb_entry(uint64_t virt) {
  // Stub
}
