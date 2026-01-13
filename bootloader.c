#include "bootloader.h"
#include "hal.h"
#include <stddef.h>
extern uint8_t __kernel_start;
extern uint8_t __kernel_end;
extern const uint32_t __kernel_checksum;
extern const void __start_agents;
extern const void __stop_agents;
extern void kernel_init(void);

#define CRC32_POLY 0xEDB88320

static uint32_t calculate_crc32(const uint8_t *data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC32_POLY;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

/* --- Bootloader Main --- */
void bootloader_main(void) {
    // 1. Calculate Kernel Integrity
    // We calculate CRC over the text/data sections of the kernel
    uint32_t kernel_size = (uint32_t)&__kernel_end - (uint32_t)&__kernel_start;
    uint32_t calculated_crc = calculate_crc32(&__kernel_start, kernel_size);

    // Compare calculated CRC with the expected checksum symbol value
    // Note: (uint32_t)&__kernel_checksum retrieves the value assigned to the symbol in the linker script
    bool integrity_pass = (calculated_crc == (uint32_t)&__kernel_checksum);

    // 2. Verify Agent Manifests
    // Ensure the agents section is not empty and symbols are valid pointers
    bool agents_valid = (&__start_agents != NULL) && 
                        (&__stop_agents != NULL) && 
                        (&__start_agents < &__stop_agents);

    if (integrity_pass && agents_valid) {
        // 3. Hand over control to Kernel
        // Stack pointer is already initialized by startup.s
        kernel_init();
    } else {
        // 4. Safe Recovery Mode
        // If integrity fails, we halt the CPU to prevent executing corrupt code.
        
        // Ensure interrupts are disabled before halting
        // We check if the function pointer exists to be safe, though HAL_Impl should be populated
        if (HAL_Impl.disable_interrupts) {
            HAL_Impl.disable_interrupts();
        }

        // Enter infinite Halt Loop
        while (1) {
            if (HAL_Impl.halt_cpu) {
                HAL_Impl.halt_cpu();
            }
        }
    }
}