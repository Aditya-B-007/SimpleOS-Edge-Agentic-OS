#include "kernel.h"
#include "agent_rt.h"
extern void arch_disable_interrupts(void);
extern void arch_enable_interrupts(void);
extern void arch_halt_cpu(void);
extern void arch_context_switch(uint32_t** old_sp, uint32_t* new_sp);
extern void arch_mpu_load_regions(Region* regions);
extern void arch_wait_for_interrupt(void);
extern void arch_init_mpu(void);

// Linker Symbols for the .agents section
extern const AgentManifest __start_agents;
extern const AgentManifest __stop_agents;

// Architecture specific stack setup (Mock)
extern uint32_t* arch_stack_init(void (*task_func)(void), uint32_t *stack_mem, uint32_t size);
static uint32_t g_agent_stacks[MAX_AGENTS][1024]; // Increased pool to support Supervisor stack

//GLOBAL KERNEL STATES
static bool interrupts_enabled = false;
static Agent g_agents[MAX_AGENTS];
static uint8_t g_agent_count = 0;
static Agent *g_current_agent = NULL;
static bool g_scheduler_lock = false;
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
volatile uint32_t g_system_ticks = 0;

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
    uint32_t last_tick = agent_get_time_ms();

    // 1. Call the user's Init function
    if (meta->api->on_init) {
        meta->api->on_init();
    }

    // 2. Enter the Event Loop
    Message msg;
    while (1) {
        bool active = false;

        // Non-blocking check for messages
        if (sys_receive_message(&msg) == 0) {
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

//KERNEL INITIALIZATION
void kernel_init(void){
    arch_init_mpu();
    memset(g_agents, 0, sizeof(g_agents));
    g_agent_count = 0;
    g_current_agent = NULL;
    g_scheduler_lock = false;
    interrupts_enabled = true;

    // Manually register the Supervisor Agent (Bypassing linker section)
    register_supervisor();
    register_netstack();

    // Iterate over the .agents section
    const AgentManifest *iter = &__start_agents;
    const AgentManifest *end = &__stop_agents;

    while (iter < end) {
        if (iter->id >= MAX_AGENTS) { iter++; continue; }
        if (g_agents[iter->id].manifest != NULL) { iter++; continue; }

        Agent *ag = &g_agents[iter->id];
        ag->id = iter->id;
        ag->priority = iter->priority;
        ag->state = STATE_READY;
        ag->manifest = iter;
        ag->stack_ptr = arch_stack_init(agent_runner, g_agent_stacks[iter->id], iter->stack_size);
        ag->caps = iter->caps; // Copy capabilities from ROM to RAM

        g_agent_count++;
        iter++;
    }
    arch_enable_interrupts();
}
void kernel_register_agent(const Agent* manifest){
    if(g_agent_count >= MAX_AGENTS) return;
    g_agents[g_agent_count] = *manifest;
    g_agents[g_agent_count].id = g_agent_count;
    g_agent_count++;
}
//KERNEL SCHEDULER - THE HEARTBEAT
void kernel_scheduler(void) {
    if (g_scheduler_lock) return;
    g_scheduler_lock = true;
    Agent *next_agent = NULL;
    uint8_t highest_prio = 255;
    // 1. Scan for Highest Priority READY Agent
    // Optimization: Use a bitmap for O(1) lookup in production
    for (int i = 0; i < MAX_AGENTS; i++) {
        if (g_agents[i].state == STATE_READY && g_agents[i].priority < highest_prio) {
            highest_prio = g_agents[i].priority;
            next_agent = &g_agents[i];
        }
    }
    // 2. Battery IoT Adaptation: Tickless Idle
    // If no agent is ready, we do not spin. We sleep.
    if (next_agent == NULL) {
        g_current_agent = NULL; // No one owns the CPU
        g_scheduler_lock = false;
        // Processor enters deep sleep until Hardware Interrupt (IRQ) fires
        arch_wait_for_interrupt(); 
        // Upon waking, an ISR will queue a message, making an Agent READY.
        // We then recurse or loop back to scheduler.
        return; 
    }
    if (g_current_agent != NULL && 
        g_current_agent->state == STATE_RUNNING && 
        g_current_agent->priority <= next_agent->priority) {
        g_scheduler_lock = false;
        return; 
    }
    // 4. Context Switch
    Agent *prev_agent = g_current_agent;
    g_current_agent = next_agent;
    g_current_agent->state = STATE_RUNNING;
    // Enforce Safety: Re-program MPU for the new agent
    arch_mpu_load_regions(g_current_agent->memory_map);
    // Swap Stacks (Arch specific assembly)
    if (prev_agent) {
        arch_context_switch(&prev_agent->stack_ptr, g_current_agent->stack_ptr);
    } else {
        // First boot or waking from idle
        arch_context_switch(NULL, g_current_agent->stack_ptr);
    }
    g_scheduler_lock = false;
}

// SECURITY: Check if agent has permission for a specific object
static bool k_has_capability(Agent *agent, ObjectType type, uint32_t target_id, uint32_t required_perms) {
    // 1. Check Global Permissions (Superuser/Admin)
    if (agent->caps.global_perms & PERM_ADMIN) return true;

    // 2. Check Specific Capability List (ACL)
    for (int i = 0; i < 16; i++) {
        KernelObject *obj = &agent->caps.c_list[i];
        if (obj->type == OBJ_TYPE_NONE) break; // End of list
        
        if (obj->type == type && obj->target_id == target_id) {
            // Check if we have the required permission bits
            if ((obj->access_rights & required_perms) == required_perms) return true;
        }
    }
    return false;
}

//IPC: SEND MESSAGE TO ANOTHER AGENT
int sys_send_message(uint32_t target_id, uint32_t signal, uintptr_t data) {
    if (target_id >= MAX_AGENTS) return -1; // Invalid Target
    Agent *target_agent = &g_agents[target_id];
    Agent *current = g_current_agent;
    Message msg;
    msg.sender_id = g_current_agent ? g_current_agent->id : 0xFFFFFFFF; // 0xFFFFFFFF for system
    msg.signal_type = signal;
    msg.payload = data;
    if (!enqueue_msg(target_agent, &msg)) {
        return -2; // Mailbox Full
    }
    // If target was DORMANT or BLOCKED, make it READY
    if (target_agent->state == STATE_DORMANT || target_agent->state == STATE_BLOCKED) {
        target_agent->state = STATE_READY;
    }
    // Check for ACCESS_SEND permission
    if (current && !k_has_capability(current, OBJ_TYPE_AGENT, target_id, ACCESS_SEND)) {
        agent_log("[SEC] IPC Violation: Agent %d tried to send to %d", current->id, target_id);
        return -2; // E_ACCESS_DENIED
    }
    return 0; // Success
}
//IPC: RECEIVE MESSAGE FROM ANOTHER AGENT
int sys_receive_message(Message* out_msg) {
    if (g_current_agent == NULL) return -1; // No current agent
    Agent *agent = g_current_agent;
    if (agent->mailbox_count == 0) {
        return -2; // No messages
    }
    *out_msg = agent->mailbox[agent->mailbox_head];
    agent->mailbox_head = (agent->mailbox_head + 1) % MAX_MAILBOX_DEPTH;
    agent->mailbox_count--;
    return 0; // Success
}
//ENABLE INTERRUPTS
void enable_interrupts(void){
    if(!interrupts_enabled){
        arch_enable_interrupts();
        interrupts_enabled = true;
    }
}
//DISABLE INTERRUPTS
void disable_interrupts(void){
    if(interrupts_enabled){
        arch_disable_interrupts();
        interrupts_enabled = false;
    }
}
//HALT CPU
void halt_cpu(void){
    arch_halt_cpu();
}
void sys_yield(void){
    if(g_current_agent != NULL){
        g_current_agent->state = STATE_READY;
    }
    kernel_scheduler();
}

void panic(const char* msg) {
    arch_disable_interrupts();
    agent_log("[KERNEL PANIC] %s", msg);
    while (1) {
        arch_halt_cpu();
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
    while(1) arch_halt_cpu();
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

static void register_supervisor(void) {
    uint32_t slot = supervisor_manifest.id;
    if (slot < MAX_AGENTS) {
        Agent *ag = &g_agents[slot];
        ag->id = slot;
        ag->priority = supervisor_manifest.priority;
        ag->state = STATE_READY;
        ag->manifest = &supervisor_manifest;
        // Use the stack slot corresponding to the ID to avoid overlap
        ag->stack_ptr = arch_stack_init(agent_runner, g_agent_stacks[slot], supervisor_manifest.stack_size);
        ag->caps = supervisor_manifest.caps;
        g_agent_count++;
    } else {
        panic("Critical Failure: Supervisor Agent ID exceeds MAX_AGENTS");
    }
}
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

static void register_netstack(void) {
    uint32_t slot = net_manifest.id;
    if (slot < MAX_AGENTS) {
        Agent *ag = &g_agents[slot];
        ag->id = slot;
        ag->priority = net_manifest.priority;
        ag->state = STATE_READY;
        ag->manifest = &net_manifest;
        // Use the stack slot corresponding to the ID to avoid overlap
        ag->stack_ptr = arch_stack_init(agent_runner, g_agent_stacks[slot], net_manifest.stack_size);
        ag->caps = net_manifest.caps;
        g_agent_count++;
    } else {
        panic("Critical Failure: NetStack Agent ID exceeds MAX_AGENTS");
    }
}
