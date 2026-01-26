#include "integrator.h"
#include "hal.h"
#include "integrator_internal.h"
#include "ipc.h"
#include "threads.h"
#include <stddef.h>

static SubsystemRegistry g_registry;
static RoutingTable g_routing_table;

/**
 * @brief Prepares a HardwareContext from the IntegratorContext.
 *
 * Initializes all relevant fields of the HardwareContext structure
 * using values from the IntegratorContext to ensure safe HAL operations.
 * @param hal_ctx Pointer to the HardwareContext to initialize.
 * @param integ_ctx Pointer to the source IntegratorContext.
 */
static void prepare_hardware_context(HardwareContext *hal_ctx,
                                     IntegratorContext *integ_ctx) {
  if (!hal_ctx)
    return;
  hal_ctx->cpu_architecture = integ_ctx ? integ_ctx->cpu_architecture : 0;
  hal_ctx->cpu_id = 0;                    // Primary CPU
  hal_ctx->interrupt_controller_type = 0; // Default/GIC
  hal_ctx->timer_type = 0;                // Default timer
  hal_ctx->boot_info_address = integ_ctx ? integ_ctx->boot_info_address : 0;
  hal_ctx->kernel_memory_base = integ_ctx ? integ_ctx->kernel_memory_base : 0;
  hal_ctx->kernel_memory_limit = integ_ctx ? integ_ctx->kernel_memory_limit : 0;
}

void integrator_initialize_hardware_layer(IntegratorContext *context,
                                          IntegratorTransaction *transaction) {
  if (!context || !transaction)
    return;

  transaction->current_phase = INTEGRATOR_PHASE_HAL_INIT;

  if (!integrator_internal_validate_boot_environment(context)) {
    transaction->status_code = INTEGRATOR_STATUS_FAILURE;
    transaction->failure_reason_code = INTEGRATOR_FAIL_HAL; // broad category
    return;
  }

  HardwareContext hal_ctx;
  hal_ctx.cpu_architecture = context->cpu_architecture;
  hal_ctx.boot_info_address = context->boot_info_address;

  HardwareTransaction hal_txn;
  hal_txn.operation_code = HAL_OP_INIT_HARDWARE;

  hal_initialize_hardware(&hal_ctx, &hal_txn);

  if (hal_txn.status_code != HAL_STATUS_OK) {
    transaction->status_code = INTEGRATOR_STATUS_FAILURE;
    transaction->failure_reason_code = INTEGRATOR_FAIL_HAL;
    return;
  }

  g_registry.hal_instance = (void *)1;

  transaction->status_code = INTEGRATOR_STATUS_OK;
}

void integrator_initialize_trap_routing(IntegratorContext *context,
                                        IntegratorTransaction *transaction) {
  if (!context || !transaction)
    return;

  transaction->current_phase = INTEGRATOR_PHASE_TRAP_INIT;

  g_registry.trap_instance = (void *)1;

  transaction->status_code = INTEGRATOR_STATUS_OK;
}

void integrator_initialize_thread_subsystem(
    IntegratorContext *context, IntegratorTransaction *transaction) {
  if (!context || !transaction)
    return;

  transaction->current_phase = INTEGRATOR_PHASE_THREAD_INIT;

  g_registry.thread_instance = (void *)1;

  transaction->status_code = INTEGRATOR_STATUS_OK;
}

void integrator_initialize_ipc_subsystem(IntegratorContext *context,
                                         IntegratorTransaction *transaction) {
  if (!context || !transaction)
    return;

  transaction->current_phase = INTEGRATOR_PHASE_IPC_INIT;

  g_registry.ipc_instance = (void *)1;

  transaction->status_code = INTEGRATOR_STATUS_OK;
}

void integrator_initialize_syscall_dispatching(
    IntegratorContext *context, IntegratorTransaction *transaction) {
  if (!context || !transaction)
    return;

  transaction->current_phase = INTEGRATOR_PHASE_SYSCALL_INIT;

  g_registry.syscall_instance = (void *)1;

  transaction->status_code = INTEGRATOR_STATUS_OK;
}

void integrator_wire_microkernel_dependencies(
    IntegratorContext *context, IntegratorTransaction *transaction) {
  if (!context || !transaction)
    return;

  transaction->current_phase = INTEGRATOR_PHASE_WIRING;

  integrator_internal_bind_ipc_to_thread_ports();
  integrator_internal_bind_thread_to_hal_ports();
  integrator_internal_bind_trap_to_hal_ports();
  integrator_internal_bind_trap_to_syscall_entry();

  transaction->status_code = INTEGRATOR_STATUS_OK;
}

void integrator_activate_interrupts_and_timer_tick(
    IntegratorContext *context, IntegratorTransaction *transaction) {
  if (!context || !transaction)
    return;

  transaction->current_phase = INTEGRATOR_PHASE_ACTIVATION;

  HardwareContext hal_ctx;
  HardwareTransaction hal_txn;

  // Initialize HardwareContext from IntegratorContext
  prepare_hardware_context(&hal_ctx, context);

  hal_txn.operation_code = HAL_OP_SET_TIMER;
  hal_txn.input_value = 1000; // 1000Hz ticks
  hal_txn.input_address = 0;
  hal_txn.output_address = 0;
  hal_txn.status_code = HAL_STATUS_OK;
  hal_configure_timer_tick(&hal_ctx, &hal_txn);

  hal_txn.operation_code = HAL_OP_ENABLE_IRQ;
  hal_enable_interrupts(&hal_ctx, &hal_txn);

  transaction->status_code = INTEGRATOR_STATUS_OK;
}

void integrator_launch_initial_system_thread(
    IntegratorContext *context, IntegratorTransaction *transaction) {
  if (!context || !transaction)
    return;

  ThreadDescriptor t_ctx;
  t_ctx.thread_id = context->initial_thread_id;
  t_ctx.owner_agent_id = context->initial_agent_id;

  ThreadTransaction t_txn;
  t_txn.action = THREAD_ACTION_CREATE;

  create_thread(&t_ctx, &t_txn);

  integrator_internal_set_kernel_phase_ready(transaction);
}

bool integrator_internal_validate_boot_environment(IntegratorContext *ctx) {
  if (!ctx)
    return false;
  if (ctx->kernel_memory_limit <= ctx->kernel_memory_base)
    return false;
  return true;
}

void integrator_internal_allocate_kernel_internal_memory(
    IntegratorContext *ctx) {
  // Stub
}

void integrator_internal_bind_ipc_to_thread_ports(void) {
  // Stub: IPC module -> Thread module
}

void integrator_internal_bind_thread_to_hal_ports(void) {
  // Stub: Thread module -> HAL context save/restore
}

void integrator_internal_bind_trap_to_hal_ports(void) {
  // Stub: Trap module -> HAL ack/mask
}

void integrator_internal_bind_trap_to_syscall_entry(void) {
  // Stub: Trap vector -> Syscall handler
}

void integrator_internal_set_kernel_phase_ready(IntegratorTransaction *txn) {
  if (txn) {
    txn->current_phase = INTEGRATOR_PHASE_READY;
    txn->status_code = INTEGRATOR_STATUS_OK;
  }
}
