#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint64_t boot_info_address;   // Physical address of boot info
  uint32_t cpu_architecture;    // 0=x86_64, 1=ARM64, etc.
  uint32_t cpu_count;           // Number of detected CPUs
  uint64_t kernel_memory_base;  // Start of kernel memory
  uint64_t kernel_memory_limit; // End of kernel memory
  uint32_t initial_agent_id;    // ID for the init process/agent
  uint32_t initial_thread_id;   // ID for the init thread
} IntegratorContext;

typedef enum {
  INTEGRATOR_PHASE_HAL_INIT = 1,
  INTEGRATOR_PHASE_TRAP_INIT = 2,
  INTEGRATOR_PHASE_THREAD_INIT = 3,
  INTEGRATOR_PHASE_IPC_INIT = 4,
  INTEGRATOR_PHASE_SYSCALL_INIT = 5,
  INTEGRATOR_PHASE_WIRING = 6,
  INTEGRATOR_PHASE_ACTIVATION = 7,
  INTEGRATOR_PHASE_READY = 8
} kIntegratorPhase;

typedef enum {
  INTEGRATOR_STATUS_OK = 0,
  INTEGRATOR_STATUS_FAILURE = 1
} kIntegratorStatusCode;

typedef enum {
  INTEGRATOR_FAIL_NONE = 0,
  INTEGRATOR_FAIL_HAL = 1,
  INTEGRATOR_FAIL_TRAP = 2,
  INTEGRATOR_FAIL_THREAD = 3,
  INTEGRATOR_FAIL_IPC = 4,
  INTEGRATOR_FAIL_SYSCALL = 5,
  INTEGRATOR_FAIL_WIRING = 6
} kIntegratorFailureCode;

typedef struct {
  uint32_t current_phase;       // kIntegratorPhase
  uint32_t status_code;         // kIntegratorStatusCode
  uint32_t failure_reason_code; // kIntegratorFailureCode
} IntegratorTransaction;

void integrator_initialize_hardware_layer(IntegratorContext *context,
                                          IntegratorTransaction *transaction);
void integrator_initialize_trap_routing(IntegratorContext *context,
                                        IntegratorTransaction *transaction);
void integrator_initialize_thread_subsystem(IntegratorContext *context,
                                            IntegratorTransaction *transaction);
void integrator_initialize_ipc_subsystem(IntegratorContext *context,
                                         IntegratorTransaction *transaction);
void integrator_initialize_syscall_dispatching(
    IntegratorContext *context, IntegratorTransaction *transaction);
void integrator_wire_microkernel_dependencies(
    IntegratorContext *context, IntegratorTransaction *transaction);
void integrator_activate_interrupts_and_timer_tick(
    IntegratorContext *context, IntegratorTransaction *transaction);
void integrator_launch_initial_system_thread(
    IntegratorContext *context, IntegratorTransaction *transaction);
void integrator_internal_set_kernel_phase_ready(IntegratorTransaction *txn);

#endif // INTEGRATOR_H
