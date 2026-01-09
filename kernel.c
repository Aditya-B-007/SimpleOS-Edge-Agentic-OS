#include "kernel.h"
#include "agent_rt.h"
#include "hal.h"
#include "mem_manager.h"
#include "ipc_manager.h"
#include "scheduler.h"
#include "irq_manager.h"
#include "agent_factory.h"

extern void arch_wait_for_interrupt(void);
extern void arch_init_mpu(void);

// Linker Symbols for the .agents section
extern const AgentManifest __start_agents;
extern const AgentManifest __stop_agents;

// Architecture specific stack setup (Mock)
extern uint32_t* arch_stack_init(void (*task_func)(void), uint32_t *stack_mem, uint32_t size);

//GLOBAL KERNEL STATES
static bool interrupts_enabled = false;
static Agent g_agents[MAX_AGENTS];
static uint8_t g_agent_count = 0;
static Agent *g_current_agent = NULL;
static bool g_scheduler_lock = false;
static uint32_t g_system_ticks = 0;
Microkernel_Object Kernel;
static void register_supervisor(void);
static void register_netstack(void);
static bool enqueue_msg(Agent *dest, Message *msg) {
    if (dest->mailbox_count >= MAX_MAILBOX_DEPTH) return false;
    dest->mailbox[dest->mailbox_tail] = *msg;
    dest->mailbox_tail = (dest->mailbox_tail + 1) % MAX_MAILBOX_DEPTH;
    dest->mailbox_count++;
    return true;
}
int sys_receive_message(Message* out_msg);

uint32_t agent_get_time_ms(void) {
    return g_system_ticks;
}

void agent_log(const char* fmt, ...) {
    // Stub: Implement UART logging here
}

int agent_send(uint32_t target_agent_id, uint32_t signal, void* data) {
    return sys_send_message(target_agent_id, signal, (uintptr_t)data);
}

// The "Trampoline" that every agent thread runs
void agent_runner(void) {
    if (!g_current_agent || !g_current_agent->manifest) return;
    
    const AgentManifest *meta = (const AgentManifest *)g_current_agent->manifest;
    uint32_t last_tick = 0;

    // 1. Call the user's Init function
    if (meta->api->on_init) {
        meta->api->on_init();
    }

    // 2. Enter the Event Loop
    Message msg;
    while (1) {
        bool active = false; 

        // Non-blocking check for messages
        if (sys_receive_message(&msg) == SYS_OK) {
            switch (msg.signal_type) {
                case SIG_SYS_FAULT:
                    if (meta->api->on_fault) {
                        meta->api->on_fault((uint32_t)msg.payload);
                    }
                    break;
                
                default:
                    // Standard IPC
                    if (meta->api->on_message) {
                        meta->api->on_message(&msg);
                    }
                    break;
            }
            active = true;
        }

        // Check for periodic tick (10ms interval)
        if (meta->api->on_tick) {
            uint32_t now = agent_get_time_ms();
            if (now - last_tick >= 10) {
                meta->api->on_tick();
                last_tick = now;
                active = true;
            }
        }

        if (!active) {
            sys_yield(); // Yield if no work
        }
    }
}

//ENABLE INTERRUPTS
void enable_interrupts(void){
    if(!interrupts_enabled){
        HAL_Impl.enable_interrupts();
        interrupts_enabled = true;
    }
}
//DISABLE INTERRUPTS
void disable_interrupts(void){
    if(interrupts_enabled){
        HAL_Impl.disable_interrupts();
        interrupts_enabled = false;
    }
}
//HALT CPU
void halt_cpu(void){
    HAL_Impl.halt_cpu();
}
void sys_yield(void){
    if(g_current_agent != NULL){
        g_current_agent->state = STATE_READY;
    }
    Kernel.scheduler->schedule_next();
}

void panic(const char* msg) {
    HAL_Impl.disable_interrupts();
    agent_log("[KERNEL PANIC] %s", msg);
    while (1) {
        HAL_Impl.halt_cpu();
    }
}

#define MAX_RESTARTS 3
#define HEARTBEAT_PERIOD_MS 1000
#define SAFE_MODE 0
static int sys_agent_spawn(const AgentManifest *manifest) {
    return 0; 
}

static void sys_agent_restart(uint32_t agent_id) {
    if (agent_id < MAX_AGENTS) {
        g_agents[agent_id].state = STATE_READY;
    }
}

static void sys_system_reset(int mode) {
    // Stub: Halt
    while(1) HAL_Impl.halt_cpu();
}

typedef struct {
    uint32_t id;
    uint32_t restart_count;
    uint32_t last_heartbeat;
    bool     is_critical;
} AgentStatus;

static AgentStatus registry[MAX_AGENTS];
static uint32_t registered_count = 0;

void supervisor_init(void) {
    agent_log("[SYS] Supervisor Starting...");
    const AgentManifest *curr = &__start_agents;
    
    while (curr < &__stop_agents) {
        if (curr->id != 0 && curr->id != 1) { 
            int res = sys_agent_spawn(curr);
            
            if (res == 0) {
                // Track it locally
                registry[registered_count].id = curr->id;
                registry[registered_count].restart_count = 0;
                registry[registered_count].is_critical = (curr->priority < 10);
                registered_count++;
                
                agent_log("[SYS] Spawned Agent: %s (ID: %d)", curr->name, curr->id);
            } else {
                agent_log("[ERR] Failed to spawn %s", curr->name);
            }
        }
        curr++; // Move to next manifest in Flash
    }
    
    agent_log("[SYS] Boot Complete. System Running.");
}

void supervisor_monitor(void) {
    // 2. WATCHDOG: Check Heartbeats
    uint32_t now = agent_get_time_ms();
    
    for (int i=0; i < registered_count; i++) {
        if (now - registry[i].last_heartbeat > 3000) {
            agent_log("[WARN] Agent %d unresponsive!", registry[i].id);
            agent_send(registry[i].id, SIG_SYS_PING, 0);
        }
    }
}

void supervisor_handle_fault(Message *msg) {
    uint32_t dead_agent_id = msg->sender_id;
    uint32_t fault_reason = msg->payload;

    agent_log("[FATAL] Agent %d Died! Reason: 0x%X", dead_agent_id, fault_reason);

    // Find agent in our registry
    AgentStatus *status = NULL;
    for(int i=0; i<registered_count; i++) {
        if (registry[i].id == dead_agent_id) {
            status = &registry[i];
            break;
        }
    }

    if (!status) return;// Unknown agent
    if (status->restart_count < MAX_RESTARTS) {
        // STRATEGY A: Attempt Restart
        status->restart_count++;
        sys_agent_restart(dead_agent_id);
        agent_log("[SYS] Restarting Agent %d (Attempt %d/%d)", 
                  dead_agent_id, status->restart_count, MAX_RESTARTS);
    } 
    else {
        // STRATEGY B: Give Up
        if (status->is_critical) {
            // CRITICAL FAILURE: Industrial Safety Stop
            agent_log("[EMERG] Critical Agent Failed. HALTING SYSTEM.");
            sys_system_reset(SAFE_MODE);
        } else {
            // NON-CRITICAL: Just disable it (IoT/Vision)
            agent_log("[SYS] Agent %d disabled permanently.", dead_agent_id);
        }
    }
}

/* --- REGISTRATION --- */
static const AgentHandlers supervisor_api = {
    .on_init = supervisor_init,
    .on_tick = supervisor_monitor,
    .on_message = supervisor_handle_fault, // Kernel sends FAULT msgs here
};

static const AgentManifest supervisor_manifest = {
    .name = "Supervisor",
    .id = 1,
    .stack_size = 2048,
    .priority = 0,
    .api = &supervisor_api,
    .caps = { .global_perms = PERM_ADMIN } // Supervisor gets full access
};

static void net_init(void) { agent_log("NetStack Started"); }
static void net_msg(Message* m) { agent_log("NetStack Msg: %d", m->signal_type); }

static const AgentHandlers net_handlers = { .on_init = net_init, .on_message = net_msg };

static const AgentManifest net_manifest = {
    .name = "NetStack",
    .id = ID_NET,
    .stack_size = 2048,
    .priority = 2,
    .api = &net_handlers,
    .caps = {
        .global_perms = PERM_LOGGING,
        .c_list = {
            { OBJ_TYPE_AGENT, ID_CLOUD_UPLINK, ACCESS_SEND },
            { OBJ_TYPE_IRQ,   IRQ_ETH_RX,      ACCESS_BIND },
            { OBJ_TYPE_REGION, REGION_ETH_DMA, ACCESS_READ | ACCESS_WRITE },
            { 0 }
        }
    }
};

//KERNEL INITIALIZATION
void kernel_init(void){
    arch_init_mpu();
    
    // Initialize the Kernel Object with Singletons
    Kernel.hal = &HAL_Impl;
    Kernel.mm = &MemMgr_Impl;
    Kernel.ipc = &IPC_Impl;
    Kernel.scheduler = &Scheduler_Impl;
    Kernel.interrupts = &IRQMgr_Impl;
    Kernel.factory = &AgentFactory_Impl;

    // Spawn Supervisor
    AgentProfile supervisor_profile = {
        .id = supervisor_manifest.id,
        .priority = supervisor_manifest.priority,
        .caps = supervisor_manifest.caps,
        .stack_size = supervisor_manifest.stack_size,
        .manifest = &supervisor_manifest
    };
    Agent* supervisor = Kernel.factory->spawn_agent(&supervisor_profile);
    Kernel.scheduler->add_task(supervisor);

    // Spawn NetStack
    AgentProfile net_profile = {
        .id = net_manifest.id,
        .priority = net_manifest.priority,
        .caps = net_manifest.caps,
        .stack_size = net_manifest.stack_size,
        .manifest = &net_manifest
    };
    Agent* netstack = Kernel.factory->spawn_agent(&net_profile);
    Kernel.scheduler->add_task(netstack);

    Kernel.hal->enable_interrupts();

    // Main Loop
    while (1) {
        Kernel.scheduler->schedule_next();
    }
}