#include "mem_manager.h"

// External architecture declaration
extern void arch_mpu_load_regions(Region* regions);

// Private memory pool moved from kernel.c
static uint32_t g_agent_stacks[MAX_AGENTS][1024];
static bool g_stack_slots_used[MAX_AGENTS] = {false};

static uint32_t* mm_allocate_stack(size_t size) {
    // Simple first-fit allocator
    // Ensure requested size fits in the fixed slot (1024 words = 4096 bytes)
    if (size > sizeof(g_agent_stacks[0])) {
        return NULL;
    }

    for (int i = 0; i < MAX_AGENTS; i++) {
        if (!g_stack_slots_used[i]) {
            g_stack_slots_used[i] = true;
            return g_agent_stacks[i];
        }
    }
    return NULL;
}

static void mm_lock_region(Region* r) {
    arch_mpu_load_regions(r);
}

const MemoryManager_Interface MemMgr_Impl = {
    .allocate_stack = mm_allocate_stack,
    .lock_region = mm_lock_region
};