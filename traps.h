#ifndef TRAPS_H
#define TRAPS_H
#include <stddef.h>
#include <stdint.h>
#define TRAP_SUCCESS 0
#define TRAP_ERR_INVALID_PARAM -1
#define TRAP_ERR_NOT_SUPPORTED -2
#define TRAP_TYPE_SYSCALL 1
#define TRAP_TYPE_INTERRUPT 2
#define TRAP_TYPE_FAULT 3
#define TRAP_DISPATCH_OK 0
#define TRAP_DISPATCH_UNHANDLED 1
#define TRAP_DISPATCH_THREAD_KILLED 2
#define TRAP_RETURN_TO_CALLER 0
#define TRAP_RETURN_SCHEDULE_NEXT 1
#define TRAP_RETURN_PANIC 2
typedef struct {
  uint32_t trap_type;
  uint32_t trap_number;
  uint32_t cpu_id;
  uint32_t privilege_level;
  uint32_t current_thread_id;
  uint32_t current_agent_id;
} TrapContext;
typedef struct {
  void *trap_frame_address;
  uint32_t error_code;
  int32_t dispatch_status;
  uint32_t return_action;
} TrapTransaction;
int capture_trap_state(TrapContext *ctx, TrapTransaction *txn);
int dispatch_trap(TrapContext *ctx, TrapTransaction *txn);
int handle_fault_trap(TrapContext *ctx, TrapTransaction *txn);
int handle_interrupt_trap(TrapContext *ctx, TrapTransaction *txn);
int handle_syscall_trap(TrapContext *ctx, TrapTransaction *txn);
int restore_trap_state_and_return(TrapContext *ctx, TrapTransaction *txn);

#endif
