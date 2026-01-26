#include "ipc.h"
#include <stdlib.h>
#include <string.h>

typedef struct MessageNode {
  MessageEnvelope envelope;
  void *internal_payload_copy;
  struct MessageNode *next;
} MessageNode;

typedef struct {
  ChannelDescriptor descriptor;
  MessageNode *head;
  MessageNode *tail;
  uint32_t current_count;
  int is_active;
} Channel;

#define MAX_CHANNELS 1024
static Channel channel_table[MAX_CHANNELS];
static int initialized = 0;

static void ensure_initialized() {
  // TODO: This initialization pattern is not thread-safe.
  // In a multi-threaded environment, use atomic operations or locking.
  if (!initialized) {
    memset(channel_table, 0, sizeof(channel_table));
    initialized = 1;
  }
}

static Channel *ipc_lookup_channel(uint32_t channel_id) {
  if (channel_id >= MAX_CHANNELS)
    return NULL;
  if (!channel_table[channel_id].is_active)
    return NULL;
  return &channel_table[channel_id];
}

static int ipc_validate_sender_permissions(Channel *chan, uint32_t src_id) {
  // permissions == 0 means public channel (any agent allowed)
  // permissions == owner_agent_id means only owner can access
  // permissions == specific_id means only that specific agent can access
  if (chan->descriptor.permissions == 0) {
    return 1; // Public channel - allow all
  }
  if (chan->descriptor.owner_agent_id == src_id) {
    return 1; // Owner always has access
  }
  if (chan->descriptor.permissions == src_id) {
    return 1; // Explicitly permitted agent
  }
  return 0; // Access denied
}

static int ipc_validate_message_size(Channel *chan, uint32_t len) {
  return len <= chan->descriptor.max_message_size;
}

static int ipc_queue_is_full(Channel *chan) {
  return chan->current_count >= chan->descriptor.max_messages;
}

static int ipc_queue_is_empty(Channel *chan) {
  return chan->current_count == 0;
}

/**
 * @brief Releases memory associated with a message node.
 *
 * Frees the internal payload copy (if any) and the node itself.
 * @param node Pointer to the MessageNode to release. Can be NULL.
 */
static void release_message_node(MessageNode *node) {
  if (!node)
    return;
  if (node->internal_payload_copy) {
    free(node->internal_payload_copy);
    node->internal_payload_copy = NULL;
  }
  free(node);
}

static int ipc_queue_push(Channel *chan, MessageEnvelope *txn) {
  MessageNode *node = (MessageNode *)malloc(sizeof(MessageNode));
  if (!node)
    return IPC_ERR_OUT_OF_MEMORY;

  node->envelope = *txn;
  node->next = NULL;

  if (txn->payload && txn->payload_len > 0) {
    node->internal_payload_copy = malloc(txn->payload_len);
    if (!node->internal_payload_copy) {
      free(node);
      return IPC_ERR_OUT_OF_MEMORY;
    }
    memcpy(node->internal_payload_copy, txn->payload, txn->payload_len);
    node->envelope.payload = node->internal_payload_copy;
  } else {
    node->internal_payload_copy = NULL;
    node->envelope.payload = NULL;
  }

  if (chan->tail) {
    chan->tail->next = node;
  } else {
    chan->head = node;
  }
  chan->tail = node;
  chan->current_count++;
  return IPC_SUCCESS;
}

static int ipc_queue_pop(Channel *chan, MessageEnvelope *txn_out) {
  if (ipc_queue_is_empty(chan))
    return IPC_ERR_CHANNEL_EMPTY;

  MessageNode *node = chan->head;
  *txn_out = node->envelope;

  chan->head = node->next;
  if (chan->head == NULL) {
    chan->tail = NULL;
  }
  chan->current_count--;

  // Release the node memory after extracting the envelope
  release_message_node(node);

  return IPC_SUCCESS;
}

int ipc_channel_create(ChannelDescriptor *ctx, MessageEnvelope *txn) {
  ensure_initialized();

  if (!ctx)
    return IPC_ERR_INVALID_PARAM;
  if (ctx->channel_id >= MAX_CHANNELS)
    return IPC_ERR_INVALID_PARAM;
  if (channel_table[ctx->channel_id].is_active)
    return IPC_ERR_PERMISSION_DENIED;
  Channel *chan = &channel_table[ctx->channel_id];
  chan->descriptor = *ctx;
  chan->head = NULL;
  chan->tail = NULL;
  chan->current_count = 0;
  chan->is_active = 1;

  return IPC_SUCCESS;
}

int ipc_channel_close(ChannelDescriptor *ctx, MessageEnvelope *txn) {
  ensure_initialized();
  if (!ctx)
    return IPC_ERR_INVALID_PARAM;

  Channel *chan = ipc_lookup_channel(ctx->channel_id);
  if (!chan)
    return IPC_ERR_CHANNEL_NOT_FOUND;

  if (chan->descriptor.owner_agent_id != txn->dst_agent_id) {
    return IPC_ERR_PERMISSION_DENIED;
  }
  MessageNode *cur = chan->head;
  while (cur) {
    MessageNode *next = cur->next;
    if (cur->internal_payload_copy)
      free(cur->internal_payload_copy);
    free(cur);
    cur = next;
  }

  chan->head = NULL;
  chan->tail = NULL;
  chan->current_count = 0;
  chan->is_active = 0;

  return IPC_SUCCESS;
}

int ipc_send(ChannelDescriptor *ctx, MessageEnvelope *txn) {
  ensure_initialized();
  if (!ctx || !txn)
    return IPC_ERR_INVALID_PARAM;

  Channel *chan = ipc_lookup_channel(ctx->channel_id);
  if (!chan)
    return IPC_ERR_CHANNEL_NOT_FOUND;

  if (!ipc_validate_sender_permissions(chan, txn->dst_agent_id)) {
    return IPC_ERR_PERMISSION_DENIED;
  }
  if (!ipc_validate_message_size(chan, txn->payload_len)) {
    return IPC_ERR_PAYLOAD_TOO_LARGE;
  }

  if (ipc_queue_is_full(chan)) {
    if (chan->descriptor.delivery_mode == IPC_DELIVERY_DROP) {
      return IPC_ERR_CHANNEL_FULL;
    } else {
      // TODO: Blocking mode requires integration with thread subsystem.
      // Should call block_thread() and wait for queue space to become
      // available. For now, return error to avoid indefinite block.
      return IPC_ERR_CHANNEL_FULL;
    }
  }

  return ipc_queue_push(chan, txn);
}

int ipc_recv(ChannelDescriptor *ctx, MessageEnvelope *txn) {
  ensure_initialized();
  if (!ctx || !txn)
    return IPC_ERR_INVALID_PARAM;

  Channel *chan = ipc_lookup_channel(ctx->channel_id);
  if (!chan)
    return IPC_ERR_CHANNEL_NOT_FOUND;

  if (ipc_queue_is_empty(chan)) {
    if (txn->flags & IPC_MSG_FLAG_NON_BLOCKING) {
      return IPC_ERR_CHANNEL_EMPTY;
    } else {
      // TODO: Blocking mode requires integration with thread subsystem.
      // Should call block_thread() and wait for message arrival.
      // For now, return error to avoid indefinite block.
      return IPC_ERR_CHANNEL_EMPTY;
    }
  }

  MessageNode *node = chan->head;

  void *user_buf = txn->payload;
  uint32_t user_buf_len = txn->payload_len;

  *txn = node->envelope;

  if (user_buf && user_buf_len > 0 && node->internal_payload_copy) {
    uint32_t copy_len = (node->envelope.payload_len < user_buf_len)
                            ? node->envelope.payload_len
                            : user_buf_len;
    memcpy(user_buf, node->internal_payload_copy, copy_len);
    txn->payload = user_buf;
    txn->payload_len = copy_len;
  } else {
  }

  chan->head = node->next;
  if (!chan->head)
    chan->tail = NULL;
  chan->current_count--;

  if (node->internal_payload_copy)
    free(node->internal_payload_copy);
  free(node);

  return IPC_SUCCESS;
}

int ipc_call(ChannelDescriptor *ctx, MessageEnvelope *txn) {
  int res = ipc_send(ctx, txn);
  if (res != IPC_SUCCESS)
    return res;

  // TODO: Synchronous call should block waiting for a reply message.
  // This requires:
  // 1. Setting IPC_MSG_FLAG_REPLY_REQUIRED on the sent message
  // 2. Blocking the calling thread via block_thread()
  // 3. Receiving the reply when woken up
  // For now, only the send portion is implemented.

  return IPC_SUCCESS;
}
