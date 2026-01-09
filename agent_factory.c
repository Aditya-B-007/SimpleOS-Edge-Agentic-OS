#include "agent_factory.h"
#include "mem_manager.h"
#include <string.h>

// External declarations for kernel functions
extern uint32_t* arch_stack_init(void (*task_func)(void), uint32_t *stack_mem, uint32_t size);
extern void agent_runner(void);

static Agent* af_spawn_agent(AgentProfile* p) {
    // Allocate memory for the Agent control block
    Agent* ag = (Agent*)kmalloc(sizeof(Agent));
    if (ag == NULL) {
        return NULL;
    }
    memset(ag, 0, sizeof(Agent));

    // Allocate stack memory
    uint32_t* stack_mem = MemMgr_Impl.allocate_stack(p->stack_size);
    if (stack_mem == NULL) {
        kfree(ag);
        return NULL;
    }

    // Initialize Agent fields
    ag->id = p->id;
    ag->priority = p->priority;
    ag->caps = p->caps;
    ag->state = STATE_DORMANT; // Not scheduled yet
    ag->manifest = p->manifest;

    // Initialize the stack context to point to the agent runner
    ag->stack_ptr = arch_stack_init(agent_runner, stack_mem, p->stack_size);

    return ag;
}

const AgentFactory_Interface AgentFactory_Impl = {
    .spawn_agent = af_spawn_agent
};