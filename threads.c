#include "threads.h"
#include <string.h>

#define MAX_THREADS 256
static ThreadDescriptor thread_table[MAX_THREADS];
static int initialized = 0;
static void ensure_initialized();
static ThreadDescriptor *lookup_thread(uint32_t id);
static void set_thread_state(ThreadDescriptor *t, uint32_t state);
static void enqueue_ready_thread(ThreadDescriptor *t);
static void dequeue_ready_thread(ThreadDescriptor *t);
static void save_thread_context(ThreadDescriptor *t);
static void restore_thread_context(ThreadDescriptor *t);
static void select_next_ready_thread();

static void ensure_initialized() {
  // TODO: This initialization pattern is not thread-safe.
  // In a multi-threaded environment, use atomic operations or locking.
  if (!initialized) {
    memset(thread_table, 0, sizeof(thread_table));
    initialized = 1;
  }
}

static ThreadDescriptor *lookup_thread(uint32_t id) {
  if (id >= MAX_THREADS)
    return NULL;
  ThreadDescriptor *t = &thread_table[id];
  if (t->state == THREAD_STATE_NEW && t->entry_point == NULL)
    return NULL;
  return t;
}

static void set_thread_state(ThreadDescriptor *t, uint32_t state) {
  if (t)
    t->state = state;
}

static void enqueue_ready_thread(ThreadDescriptor *t) {
  // Add to ready queue
}
static void dequeue_ready_thread(ThreadDescriptor *t) {
  // Remove from ready queue
}
static void save_thread_context(ThreadDescriptor *t) {
  // Save CPU registers
}
static void restore_thread_context(ThreadDescriptor *t) {
  // Restore CPU registers
}
static void select_next_ready_thread() {
  // Scheduler logic
}

int create_thread(ThreadDescriptor *ctx, ThreadTransaction *txn) {
  ensure_initialized();
  if (!ctx)
    return THREAD_ERR_INVALID_PARAM;
  if (ctx->thread_id >= MAX_THREADS)
    return THREAD_ERR_INVALID_PARAM;

  ThreadDescriptor *thread = &thread_table[ctx->thread_id];

  // Check if slot is already in use by an active thread
  if (thread->state != THREAD_STATE_NEW && thread->state != THREAD_STATE_DEAD) {
    return THREAD_ERR_PERMISSION_DENIED; // Slot already occupied
  }

  *thread = *ctx;

  set_thread_state(thread, THREAD_STATE_READY);
  enqueue_ready_thread(thread);

  if (txn)
    txn->result_code = THREAD_SUCCESS;
  return THREAD_SUCCESS;
}

int exit_thread(ThreadDescriptor *ctx, ThreadTransaction *txn) {
  ensure_initialized();
  if (!ctx)
    return THREAD_ERR_INVALID_PARAM;

  ThreadDescriptor *thread = lookup_thread(ctx->thread_id);
  if (!thread)
    return THREAD_ERR_NOT_FOUND;

  set_thread_state(thread, THREAD_STATE_DEAD);

  select_next_ready_thread();

  if (txn)
    txn->result_code = THREAD_SUCCESS;
  return THREAD_SUCCESS;
}

int yield_thread(ThreadDescriptor *ctx, ThreadTransaction *txn) {
  ensure_initialized();
  if (!ctx)
    return THREAD_ERR_INVALID_PARAM;

  ThreadDescriptor *thread = lookup_thread(ctx->thread_id);
  if (!thread)
    return THREAD_ERR_NOT_FOUND;

  // TODO: Context switch should be atomic (disable interrupts).
  // Without atomicity, re-entrancy issues may occur.
  if (thread->state == THREAD_STATE_RUNNING) {
    set_thread_state(thread, THREAD_STATE_READY);
    enqueue_ready_thread(thread);

    save_thread_context(thread);
    select_next_ready_thread();
  } else if (thread->state != THREAD_STATE_READY) {
    // Cannot yield from blocked/dead state
    return THREAD_ERR_INVALID_STATE;
  }

  if (txn)
    txn->result_code = THREAD_SUCCESS;
  return THREAD_SUCCESS;
}

int block_thread(ThreadDescriptor *ctx, ThreadTransaction *txn) {
  ensure_initialized();
  if (!ctx || !txn)
    return THREAD_ERR_INVALID_PARAM;

  ThreadDescriptor *thread = lookup_thread(ctx->thread_id);
  if (!thread)
    return THREAD_ERR_NOT_FOUND;

  // Validate state transition: can only block from RUNNING or READY
  if (thread->state != THREAD_STATE_RUNNING &&
      thread->state != THREAD_STATE_READY) {
    return THREAD_ERR_INVALID_STATE;
  }

  // TODO: Context switch should be atomic (disable interrupts).
  dequeue_ready_thread(thread);
  set_thread_state(thread, THREAD_STATE_BLOCKED);

  save_thread_context(thread);
  select_next_ready_thread();

  txn->result_code = THREAD_SUCCESS;
  return THREAD_SUCCESS;
}

int wake_thread(ThreadDescriptor *ctx, ThreadTransaction *txn) {
  ensure_initialized();
  if (!ctx)
    return THREAD_ERR_INVALID_PARAM;

  ThreadDescriptor *thread = lookup_thread(ctx->thread_id);
  if (!thread)
    return THREAD_ERR_NOT_FOUND;

  // Validate state transition: can only wake from BLOCKED
  if (thread->state != THREAD_STATE_BLOCKED) {
    // Thread is not blocked, nothing to wake
    if (txn)
      txn->result_code = THREAD_ERR_INVALID_STATE;
    return THREAD_ERR_INVALID_STATE;
  }

  set_thread_state(thread, THREAD_STATE_READY);
  enqueue_ready_thread(thread);

  if (txn)
    txn->result_code = THREAD_SUCCESS;
  return THREAD_SUCCESS;
}

int sleep_thread(ThreadDescriptor *ctx, ThreadTransaction *txn) {
  ensure_initialized();
  if (!ctx || !txn)
    return THREAD_ERR_INVALID_PARAM;

  return block_thread(ctx, txn);
}
