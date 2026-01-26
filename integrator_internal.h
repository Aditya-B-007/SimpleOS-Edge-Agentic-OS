#ifndef SIMPLEOS_INTEGRATOR_INTERNAL_H
#define SIMPLEOS_INTEGRATOR_INTERNAL_H

#include "integrator.h"
#include <stdbool.h>
#include <stdint.h>
typedef void *HalHandle;
typedef void *TrapHandle;
typedef void *SyscallHandle;
typedef void *ThreadHandle;
typedef void *IpcHandle;
typedef struct {
  HalHandle hal_instance;
  TrapHandle trap_instance;
  SyscallHandle syscall_instance;
  ThreadHandle thread_instance;
  IpcHandle ipc_instance;
} SubsystemRegistry;

typedef struct {
  void *trap_handlers[256];
  void *syscall_handlers[256];
} RoutingTable;

bool integrator_internal_validate_boot_environment(IntegratorContext *ctx);
void integrator_internal_allocate_kernel_internal_memory(
    IntegratorContext *ctx);
void integrator_internal_bind_ipc_to_thread_ports(void);
void integrator_internal_bind_thread_to_hal_ports(void);
void integrator_internal_bind_trap_to_hal_ports(void);
void integrator_internal_bind_trap_to_syscall_entry(void);

void integrator_internal_set_kernel_phase_ready(IntegratorTransaction *txn);

#endif
