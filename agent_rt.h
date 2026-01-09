#ifndef AGENT_RT_H
#define AGENT_RT_H

#include <stdint.h>
#include "kernel.h" // For Message struct

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    void (*on_init)(void); 
    void (*on_message)(Message *msg); 
    void (*on_tick)(void); 
    void (*on_fault)(uint32_t reason); 
} AgentHandlers;
typedef struct {
    const char* name;
    uint32_t id;
    uint32_t stack_size;
    uint32_t priority;
    const AgentHandlers *api;
    CapabilitySet caps; // Added: Capabilities in Manifest
} AgentManifest;

/* --- API CALLS --- */
/* Wrappers around Kernel Syscalls for ease of use */
int  agent_send(uint32_t target_agent_id, uint32_t signal, void* data);
void agent_log(const char* fmt, ...); 
uint32_t agent_get_time_ms(void);
#define REGISTER_AGENT(name_str, id_val, stack_sz, prio, handlers_struct, caps_init) \
    static const AgentManifest __attribute__((section(".agents"), used)) agent_meta = { \
        .name = name_str, \
        .id = id_val, \
        .stack_size = stack_sz, \
        .priority = prio, \
        .api = &(handlers_struct), \
        .caps = caps_init \
    };

#ifdef __cplusplus
}
#endif

#endif // AGENT_RT_H