#ifndef SYSTEM_INTERFACES_H
#define SYSTEM_INTERFACES_H

#include "kernel.h"
#include "agent_factory.h"

typedef struct HAL_Interface {
    void (*enable_interrupts)(void);
    void (*disable_interrupts)(void);
    void (*halt_cpu)(void);
    void (*context_switch)(uint32_t** old_sp, uint32_t* new_sp);
} HAL_Interface;

typedef struct MemoryManager_Interface {
    uint32_t* (*allocate_stack)(size_t size);
    void (*lock_region)(Region* r);
} MemoryManager_Interface;

typedef struct IPC_Interface {
    int (*send_message)(uint32_t target_id, uint32_t signal, uintptr_t data);
    int (*receive_message)(Message* out_msg);
} IPC_Interface;

typedef struct Scheduler_Interface {
    void (*add_task)(Agent* task);
    void (*schedule_next)(void);
} Scheduler_Interface;

typedef struct InterruptManager_Interface {
    void (*enter_critical)(void);
    void (*exit_critical)(void);
} InterruptManager_Interface;

struct Microkernel_Object {
    const HAL_Interface* hal;
    const MemoryManager_Interface* mm;
    const IPC_Interface* ipc;
    const Scheduler_Interface* scheduler;
    const InterruptManager_Interface* interrupts;
    const AgentFactory_Interface* factory;
};

#endif // SYSTEM_INTERFACES_H