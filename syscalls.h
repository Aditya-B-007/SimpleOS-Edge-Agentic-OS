#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stddef.h>
#include <stdint.h>
#define SYSCALL_SUCCESS 0
#define SYSCALL_ERR_INVALID_CONTEXT -1
#define SYSCALL_ERR_UNKNOWN_SYSCALL -2
#define SYSCALL_ERR_ACCESS_DENIED -3
#define SYSCALL_ERR_INVALID_ARGS -4
#define SYSCALL_ERR_EXECUTION_FAILED -5
#define SYSCALL_IPC_CHANNEL_CREATE 101
#define SYSCALL_IPC_CHANNEL_CLOSE 102
#define SYSCALL_IPC_SEND 103
#define SYSCALL_IPC_RECV 104
#define SYSCALL_IPC_CALL 105
#define SYSCALL_THREAD_CREATE 201
#define SYSCALL_THREAD_EXIT 202
#define SYSCALL_THREAD_YIELD 203
#define SYSCALL_THREAD_BLOCK 204
#define SYSCALL_THREAD_WAKE 205
#define SYSCALL_THREAD_SLEEP 206
#define PRIVILEGE_USER 0
#define PRIVILEGE_SERVICE 1
#define PRIVILEGE_KERNEL 2
typedef struct {
  uint32_t syscall_number;   // Requested syscall
  uint32_t caller_agent_id;  // Who called
  uint32_t caller_thread_id; // Which thread
  uint32_t abi_version;      // ABI version
  uint32_t privilege_level;  // user/kernel/service
} SyscallContext;
typedef struct {
  void *argument_block_address;   // Pointer to args
  uint32_t argument_block_size;   // Size of args
  void *return_block_address;     // Pointer to result buffer
  uint32_t return_block_max_size; // Max result size
  int32_t status_code;            // Status filled by kernel
} SyscallTransaction;

int handle_syscall(SyscallContext *ctx, SyscallTransaction *txn);

#endif 
