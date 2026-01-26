#ifndef IPC_H
#define IPC_H

#include <stdint.h>
#include <stddef.h>
#define IPC_SUCCESS 0
#define IPC_ERR_INVALID_PARAM -1
#define IPC_ERR_OUT_OF_MEMORY -2
#define IPC_ERR_PERMISSION_DENIED -3
#define IPC_ERR_CHANNEL_FULL -4
#define IPC_ERR_CHANNEL_EMPTY -5
#define IPC_ERR_CHANNEL_NOT_FOUND -6
#define IPC_ERR_TIMEOUT -7
#define IPC_ERR_PAYLOAD_TOO_LARGE -8
#define IPC_ERR_MISMATCH -9

#define IPC_DELIVERY_BLOCKING 0
#define IPC_DELIVERY_DROP 1

#define IPC_MSG_FLAG_REPLY_REQUIRED (1 << 0)
#define IPC_MSG_FLAG_NON_BLOCKING   (1 << 1)
typedef struct {
    uint32_t channel_id;      
    uint32_t owner_agent_id;  
    uint32_t channel_type;    
    uint32_t max_messages;    
    uint32_t max_message_size;
    uint32_t delivery_mode;   
    uint32_t permissions;     
    uint32_t flags;           
} ChannelDescriptor;
typedef struct {
    uint32_t msg_type;        
    uint32_t dst_agent_id;
    uint32_t corr_id;         
    uint32_t flags;           
    uint32_t payload_len;     
    void*    payload;         
} MessageEnvelope;
int ipc_channel_create(ChannelDescriptor* ctx, MessageEnvelope* txn);
int ipc_channel_close(ChannelDescriptor* ctx, MessageEnvelope* txn);
int ipc_send(ChannelDescriptor* ctx, MessageEnvelope* txn);
int ipc_recv(ChannelDescriptor* ctx, MessageEnvelope* txn);
int ipc_call(ChannelDescriptor* ctx, MessageEnvelope* txn);

#endif 
