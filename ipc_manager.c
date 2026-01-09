#include "ipc_manager.h"
#include "agent_rt.h"

// External kernel state
extern Agent* g_agent_registry[MAX_AGENTS];
extern Agent* g_current_agent;

static bool ipc_enqueue_msg(Agent *dest, Message *msg) {
    if (dest->mailbox_count >= MAX_MAILBOX_DEPTH) return false;
    dest->mailbox[dest->mailbox_tail] = *msg;
    dest->mailbox_tail = (dest->mailbox_tail + 1) % MAX_MAILBOX_DEPTH;
    dest->mailbox_count++;
    return true;
}

static bool ipc_has_capability(Agent *agent, ObjectType type, uint32_t target_id, uint32_t required_perms) {
    if (agent->caps.global_perms & PERM_ADMIN) return true;

    for (int i = 0; i < 16; i++) {
        KernelObject *obj = &agent->caps.c_list[i];
        if (obj->type == OBJ_TYPE_NONE) break;
        
        if (obj->type == type && obj->target_id == target_id) {
            if ((obj->access_rights & required_perms) == required_perms) return true;
        }
    }
    return false;
}

static int internal_send_message(Agent* sender, Agent* target, uint32_t signal, uintptr_t data) {
    // Check for ACCESS_SEND permission
    if (sender && !ipc_has_capability(sender, OBJ_TYPE_AGENT, target->id, ACCESS_SEND)) {
        agent_log("[SEC] IPC Violation: Agent %d tried to send to %d", sender->id, target->id);
        return SYS_ERR_DENIED;
    }

    Message msg;
    msg.sender_id = sender ? sender->id : 0xFFFFFFFF;
    msg.signal_type = signal;
    msg.payload = data;

    if (!ipc_enqueue_msg(target, &msg)) {
        return SYS_ERR_BOX_FULL;
    }

    if (target->state == STATE_DORMANT || target->state == STATE_BLOCKED) {
        target->state = STATE_READY;
    }
    return SYS_OK;
}

static int ipc_send_message(uint32_t target_id, uint32_t signal, uintptr_t data) {
    if (target_id >= MAX_AGENTS) return SYS_ERR_INVALID_ID;
    Agent* target = g_agent_registry[target_id];
    if (target == NULL) return SYS_ERR_INVALID_ID;
    return internal_send_message(g_current_agent, target, signal, data);
}

static int ipc_receive_message(Message* out_msg) {
    if (g_current_agent == NULL) return SYS_ERR_NO_AGENT;
    Agent *agent = g_current_agent;
    if (agent->mailbox_count == 0) return SYS_ERR_EMPTY;
    *out_msg = agent->mailbox[agent->mailbox_head];
    agent->mailbox_head = (agent->mailbox_head + 1) % MAX_MAILBOX_DEPTH;
    agent->mailbox_count--;
    return SYS_OK;
}

const IPC_Interface IPC_Impl = {
    .send_message = ipc_send_message,
    .receive_message = ipc_receive_message
};