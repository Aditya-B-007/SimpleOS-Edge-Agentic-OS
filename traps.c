#include "traps.h"
#include "hal_internal.h"
#include "syscalls.h"
#include <stdio.h>
#include <stdlib.h>

static TrapFrame *allocate_trap_frame() {
  return (TrapFrame *)malloc(sizeof(TrapFrame));
}

/**
 * @brief Releases memory associated with a trap frame.
 *
 * Safely frees the TrapFrame and sets the transaction's frame address to NULL.
 * Should be called when trap handling fails or completes.
 * @param txn Pointer to the TrapTransaction containing the frame address.
 */
static void release_trap_frame(TrapTransaction *txn) {
  if (!txn)
    return;
  TrapFrame *frame = (TrapFrame *)txn->trap_frame_address;
  if (frame) {
    free(frame);
    txn->trap_frame_address = NULL;
  }
}

static void save_registers(TrapFrame *frame) {}

static void restore_registers(TrapFrame *frame) {}

int capture_trap_state(TrapContext *ctx, TrapTransaction *txn) {
  if (!txn)
    return TRAP_ERR_INVALID_PARAM;

  TrapFrame *frame = allocate_trap_frame();
  if (!frame)
    return TRAP_ERR_INVALID_PARAM;

  save_registers(frame);
  txn->trap_frame_address = frame;

  return TRAP_SUCCESS;
}

int dispatch_trap(TrapContext *ctx, TrapTransaction *txn) {
  if (!ctx || !txn)
    return TRAP_ERR_INVALID_PARAM;

  int res = TRAP_SUCCESS;

  switch (ctx->trap_type) {
  case TRAP_TYPE_SYSCALL:
    res = handle_syscall_trap(ctx, txn);
    break;
  case TRAP_TYPE_INTERRUPT:
    res = handle_interrupt_trap(ctx, txn);
    break;
  case TRAP_TYPE_FAULT:
    res = handle_fault_trap(ctx, txn);
    break;
  default:
    txn->dispatch_status = TRAP_DISPATCH_UNHANDLED;
    txn->return_action = TRAP_RETURN_PANIC;
    // Release frame on unhandled trap before panic
    release_trap_frame(txn);
    res = TRAP_ERR_INVALID_PARAM;
  }

  return res;
}

int handle_fault_trap(TrapContext *ctx, TrapTransaction *txn) {
  txn->dispatch_status = TRAP_DISPATCH_THREAD_KILLED;
  txn->return_action = TRAP_RETURN_SCHEDULE_NEXT;

  return TRAP_SUCCESS;
}

int handle_interrupt_trap(TrapContext *ctx, TrapTransaction *txn) {
  txn->dispatch_status = TRAP_DISPATCH_OK;
  txn->return_action = TRAP_RETURN_TO_CALLER;

  return TRAP_SUCCESS;
}

int handle_syscall_trap(TrapContext *ctx, TrapTransaction *txn) {
  TrapFrame *frame = (TrapFrame *)txn->trap_frame_address;
  if (!frame)
    return TRAP_ERR_INVALID_PARAM;

  SyscallContext sys_ctx;
  sys_ctx.syscall_number = frame->x[0];
  sys_ctx.caller_agent_id = ctx->current_agent_id;
  sys_ctx.caller_thread_id = ctx->current_thread_id;
  sys_ctx.privilege_level = ctx->privilege_level;
  sys_ctx.abi_version = 1;

  SyscallTransaction sys_txn;
  sys_txn.argument_block_address = (void *)(uintptr_t)frame->x[1];
  sys_txn.argument_block_size = frame->x[2];

  int res = handle_syscall(&sys_ctx, &sys_txn);

  frame->x[0] = sys_txn.status_code;

  txn->dispatch_status = TRAP_DISPATCH_OK;
  txn->return_action = TRAP_RETURN_TO_CALLER;

  if (res != SYSCALL_SUCCESS) {
  }

  return TRAP_SUCCESS;
}

int restore_trap_state_and_return(TrapContext *ctx, TrapTransaction *txn) {
  if (!txn)
    return TRAP_ERR_INVALID_PARAM;

  TrapFrame *frame = (TrapFrame *)txn->trap_frame_address;

  if (txn->return_action == TRAP_RETURN_PANIC) {
    // Release frame before entering panic loop to avoid leak
    release_trap_frame(txn);
    while (1)
      ;
  } else if (txn->return_action == TRAP_RETURN_SCHEDULE_NEXT) {
    release_trap_frame(txn);
    return TRAP_SUCCESS;
  }

  if (frame) {
    restore_registers(frame);
    release_trap_frame(txn);
  }

  return TRAP_SUCCESS;
}
