#include "scheduler.h"
#include "hal.h"
#include "mem_manager.h"

extern Agent* g_agent_registry[MAX_AGENTS];
Agent* g_current_agent = NULL;

static void sched_add_task(Agent* task) {
    if (task && task->id < MAX_AGENTS) {
        g_agent_registry[task->id] = task;
        task->state = STATE_READY;
    }
}

static void sched_schedule_next(void) {
    Agent *next_agent = NULL;
    uint8_t highest_prio = 255;

    // Scan for Highest Priority READY Agent
    for (int i = 0; i < MAX_AGENTS; i++) {
        Agent* ag = g_agent_registry[i];
        if (ag && ag->state == STATE_READY && ag->priority < highest_prio) {
            highest_prio = ag->priority;
            next_agent = ag;
        }
    }

    if (next_agent == NULL) {
        // Idle: Halt CPU until interrupt
        g_current_agent = NULL;
        HAL_Impl.halt_cpu();
        return;
    }

    if (g_current_agent != NULL && 
        g_current_agent->state == STATE_RUNNING && 
        g_current_agent->priority <= next_agent->priority) {
        return; // Keep running current agent
    }

    Agent *prev_agent = g_current_agent;
    g_current_agent = next_agent;
    g_current_agent->state = STATE_RUNNING;

    MemMgr_Impl.lock_region(g_current_agent->memory_map);

    HAL_Impl.context_switch(prev_agent ? &prev_agent->stack_ptr : NULL, 
                            g_current_agent->stack_ptr);
}

const Scheduler_Interface Scheduler_Impl = {
    .add_task = sched_add_task,
    .schedule_next = sched_schedule_next
};