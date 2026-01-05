#ifndef KERNEL_H
#define KERNEL_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
//Configuration for the agents
#define MAX_AGENTS 16
#define MAX_REGIONS 8
#define MAX_MAILBOX_DEPTH 4
#define SCHED_TICK_HZ 1000

// SYSTEM SIGNALS
#define SIG_SYS_TICK  0xFFFFFFFE
#define SIG_SYS_FAULT 0xFFFFFFFF
#define SIG_SYS_PING  0xFFFFFFFD
#define SIG_SYS_RESET 0xFFFFFFFC

// SYSTEM RESOURCES & IDs
#define ID_NET 2
#define ID_CLOUD_UPLINK 3
#define IRQ_ETH_RX 11
#define REGION_ETH_DMA 0x40000000
#define ACCESS_READ  (1<<0)
#define ACCESS_WRITE (1<<1)
#define ACCESS_SEND  (1<<3)
#define ACCESS_BIND  (1<<4)

// STATE OF THE SYSTEM
typedef enum{
    STATE_DORMANT=0,
    STATE_RUNNING=1,
    STATE_BLOCKED=2,
    STATE_READY=3,
    STATE_FAULTED=4
} AgentState;
//CAPABILITIES
typedef enum {
    CAP_NONE=0,
    CAP_IO_ACCESS=(1<<0),
    CAP_IRQ_LISTEN=(1<<1),
    CAP_POWER_MGT=(1<<2),
    CAP_IPC_SEND=(1<<3),
    CAP_IPC_RECEIVE=(1<<4),
    CAP_MEM_MANAGE=(1<<5),
    CAP_PROC_CONTROL=(1<<6),
    CAP_ADMIN=(1<<7),
    CAP_DEBUG=(1<<8),
    CAP_FS_READ=(1<<9),
    CAP_FS_WRITE=(1<<10),
    CAP_NET_RAW=(1<<11),
    CAP_TIME_SET=(1<<12),
    CAP_SCHED_SET=(1<<13),
    CAP_DRV_LOAD=(1<<14),
    CAP_SYSLOG=(1<<15)
} Capability;
//MEMORY REGION
typedef struct{
    uintptr_t phys_addr;
    uintptr_t virtual_addr;
    size_t size;
    uint32_t permissions;
    bool enabled;
} Region;
//IPC MESSAGE
typedef struct{
    uint32_t sender_id;
    uint32_t signal_type;
    uintptr_t payload;
} Message;
//AGENT CONTROL BLOCK
typedef struct {
    uint32_t    id;
    uint8_t     priority;       // 0=Highest (Industrial), 255=Lowest
    AgentState  state;
    uint32_t    *stack_ptr;     // Context pointer (SP)
    CapabilitySet caps;         // Fixed: Use CapabilitySet to match AgentManifest
    uint8_t     mailbox_head;
    uint8_t     mailbox_tail;
    uint8_t     mailbox_count;
    Region      memory_map[MAX_REGIONS];
    Message     mailbox[MAX_MAILBOX_DEPTH];
    uint32_t    cpu_usage_cycles;
    uint32_t    fault_count;
    const void  *manifest;      // Pointer to the ROM AgentManifest
} Agent;
typedef enum {
    SAFE_MODE = 0,
    NORMAL_RESTART = 1
} ResetMode;
typedef enum {
    PERM_NONE=0,
    PERM_LOGGING=(1<<0),
    PERM_NETWORK=(1<<1),
    PERM_FILESYSTEM=(1<<2),
    PERM_HARDWARE=(1<<3),
    PERM_POWER_CTRL=(1<<4),
    PERM_RAW_DMA=(1<<5),
    PERM_DEBUG=(1<<6),
    PERM_PROC_MGMT=(1<<7),
    PERM_MEM_MGMT=(1<<8),
    PERM_IPC=(1<<9),
    PERM_TIME_MGMT=(1<<10),
    PERM_SCHED_MGMT=(1<<11),
    PERM_DRIVER_MGMT=(1<<12),
    PERM_SYSLOG=(1<<13),
    PERM_USER=(1<<14),
    PERM_ADMIN=(1<<15)
} SystemPermission;
typedef enum{
    OBJ_TYPE_NONE=0,
    OBJ_TYPE_AGENT,
    OBJ_TYPE_REGION,
    OBJ_TYPE_MESSAGE,
    OBJ_TYPE_IRQ,
    OBJ_TYPE_IO_PORT,
    OBJ_TYPE_DEVICE,
    OBJ_TYPE_FILE,
    OBJ_TYPE_SOCKET
} ObjectType;
typedef struct {
    ObjectType type;
    uint32_t   target_id;    // e.g., Agent ID or IRQ Number
    uint32_t   access_rights;// R/W/X or SEND/RECEIVE
} KernelObject;
typedef struct {
    SystemPermission global_perms;
    KernelObject     c_list[16]; // Fixed: Store Objects, not just Capability flags
} CapabilitySet;

typedef enum {
    SYS_OK = 0,
    SYS_ERR_INVALID_ID = -1,
    SYS_ERR_BOX_FULL   = -2,
    SYS_ERR_DENIED     = -3,
    SYS_ERR_EMPTY      = -4,
    SYS_ERR_NO_AGENT   = -5
} SystemError;

void kernel_init(void);
void kernel_register_agent(const Agent* manifest);
void kernel_scheduler(void); // The "Heartbeat"
int  sys_send_message(uint32_t target_id, uint32_t signal, uintptr_t data);
void sys_yield(void);
void panic(const char* msg);
#endif