#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Main entry point for the Hybrid Bootloader.
 * Performs integrity checks and hands over control to the Kernel.
 */
void bootloader_main(void);

#endif // BOOTLOADER_H