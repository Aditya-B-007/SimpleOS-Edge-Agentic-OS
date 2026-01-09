#ifndef AGENT_FACTORY_H
#define AGENT_FACTORY_H

#include "kernel.h"

typedef struct AgentProfile {
    uint32_t id;
    uint8_t priority;
    CapabilitySet caps;
    uint32_t stack_size;
    const void* manifest;
} AgentProfile;

typedef struct AgentFactory_Interface {
    Agent* (*spawn_agent)(AgentProfile* p);
} AgentFactory_Interface;

extern const AgentFactory_Interface AgentFactory_Impl;

#endif // AGENT_FACTORY_H