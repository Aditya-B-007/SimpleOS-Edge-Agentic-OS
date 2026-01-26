#include "syscalls.h"
#include "ipc.h"
#include "threads.h"
#include <string.h>
static int validate_syscall_context(SyscallContext *ctx) {
  if (!ctx)
    return 0;
  return 1;
}
static int execute_ipc_syscall(SyscallContext *ctx, SyscallTransaction *txn) {

  typedef struct {
    ChannelDescriptor cd;
    MessageEnvelope me;
  } IpcArgs;

  if (txn->argument_block_size < sizeof(IpcArgs))
    return SYSCALL_ERR_INVALID_ARGS;

  IpcArgs *args = (IpcArgs *)txn->argument_block_address;

  switch (ctx->syscall_number) {
  case SYSCALL_IPC_CHANNEL_CREATE:
    return ipc_channel_create(&args->cd, &args->me);
  case SYSCALL_IPC_CHANNEL_CLOSE:
    return ipc_channel_close(&args->cd, &args->me);
  case SYSCALL_IPC_SEND:
    return ipc_send(&args->cd, &args->me);
  case SYSCALL_IPC_RECV: {
    int res = ipc_recv(&args->cd, &args->me);
    return res;
  }
  case SYSCALL_IPC_CALL:
    return ipc_call(&args->cd, &args->me);
  default:
    return SYSCALL_ERR_UNKNOWN_SYSCALL;
  }
}

static int execute_thread_syscall(SyscallContext *ctx,
                                  SyscallTransaction *txn) {
  typedef struct {
    ThreadDescriptor td;
    ThreadTransaction tt;
  } ThreadArgs;

  if (txn->argument_block_size < sizeof(ThreadArgs))
    return SYSCALL_ERR_INVALID_ARGS;

  ThreadArgs *args = (ThreadArgs *)txn->argument_block_address;

  switch (ctx->syscall_number) {
  case SYSCALL_THREAD_CREATE:
    return create_thread(&args->td, &args->tt);
  case SYSCALL_THREAD_EXIT:
    return exit_thread(&args->td, &args->tt);
  case SYSCALL_THREAD_YIELD:
    return yield_thread(&args->td, &args->tt);
  case SYSCALL_THREAD_BLOCK:
    return block_thread(&args->td, &args->tt);
  case SYSCALL_THREAD_WAKE:
    return wake_thread(&args->td, &args->tt);
  case SYSCALL_THREAD_SLEEP:
    return sleep_thread(&args->td, &args->tt);
  default:
    return SYSCALL_ERR_UNKNOWN_SYSCALL;
  }
}

int handle_syscall(SyscallContext *ctx, SyscallTransaction *txn) {
  if (!validate_syscall_context(ctx)) {
    if (txn)
      txn->status_code = SYSCALL_ERR_INVALID_CONTEXT;
    return SYSCALL_ERR_INVALID_CONTEXT;
  }

  if (!txn)
    return SYSCALL_ERR_INVALID_ARGS;

  int result = SYSCALL_ERR_UNKNOWN_SYSCALL;

  if (ctx->syscall_number >= 100 && ctx->syscall_number < 200) {
    result = execute_ipc_syscall(ctx, txn);
  } else if (ctx->syscall_number >= 200 && ctx->syscall_number < 300) {
    result = execute_thread_syscall(ctx, txn);
  }

  txn->status_code = result;
  return result;
}
