#ifndef THREADS_H
#define THREADS_H

#include <stddef.h>
#include <stdint.h>


// Status Codes
#define THREAD_SUCCESS 0
#define THREAD_ERR_INVALID_PARAM -1
#define THREAD_ERR_OUT_OF_MEMORY -2
#define THREAD_ERR_PERMISSION_DENIED -3
#define THREAD_ERR_INVALID_STATE -4
#define THREAD_ERR_NOT_FOUND -5

// Thread States
#define THREAD_STATE_NEW 0
#define THREAD_STATE_READY 1
#define THREAD_STATE_RUNNING 2
#define THREAD_STATE_BLOCKED 3
#define THREAD_STATE_DEAD 4

// Thread Actions (for Transaction)
#define THREAD_ACTION_CREATE 1
#define THREAD_ACTION_EXIT 2
#define THREAD_ACTION_YIELD 3
#define THREAD_ACTION_BLOCK 4
#define THREAD_ACTION_WAKE 5
#define THREAD_ACTION_SLEEP 6

// Thread Descriptor (Contextual Data)
typedef struct {
  uint32_t thread_id;      // Unique identifier
  uint32_t owner_agent_id; // Owner agent
  void *entry_point;       // Function address
  void *stack_base;        // Stack base address
  uint32_t stack_size;     // Stack size
  uint32_t priority;       // Priority
  uint32_t state;          // Current state
} ThreadDescriptor;

// Thread Transaction (Transactional Data)
typedef struct {
  uint32_t action;              // Operation requested
  uint32_t requester_agent_id;  // Requesting agent
  uint32_t requester_thread_id; // Requesting thread
  uint32_t reason_code;         // Reason (e.g. IPC_WAIT)
  uint32_t timeout_ms;          // Timeout for block/sleep
  int32_t result_code;          // Result filled by kernel
} ThreadTransaction;

// Exposed Thread Methods
int create_thread(ThreadDescriptor *ctx, ThreadTransaction *txn);
int exit_thread(ThreadDescriptor *ctx, ThreadTransaction *txn);
int yield_thread(ThreadDescriptor *ctx, ThreadTransaction *txn);
int block_thread(ThreadDescriptor *ctx, ThreadTransaction *txn);
int wake_thread(ThreadDescriptor *ctx, ThreadTransaction *txn);
int sleep_thread(ThreadDescriptor *ctx, ThreadTransaction *txn);

#endif // THREADS_H
