#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// --- MEMORY MAP DEFINITIONS ---
#define ROM_BASE_ADDR   0x00000000
#define ROM_SIZE        (512 * 1024)    // 512 KB ROM (Flash / Bootloader / Program)
#define ROM_END_ADDR    (ROM_BASE_ADDR + ROM_SIZE - 1)

#define RAM_BASE_ADDR   0x20000000
#define RAM_SIZE        (128 * 1024)    // 128 KB RAM (SRAM / Stack / Heap)
#define RAM_END_ADDR    (RAM_BASE_ADDR + RAM_SIZE - 1)

#define STACK_SIZE      (8 * 1024)      // 8 KB Stack inside RAM

// --- MEMORY SYSTEM HARDWARE STRUCTURE ---
typedef struct {
    uint8_t rom[ROM_SIZE];
    uint8_t ram[RAM_SIZE];
    uint32_t stack_pointer;             // SP register
    uint32_t stack_limit;               // MPU Stack Overflow Limit
} MemorySystem;

static MemorySystem sys_mem;

// --- ADDRESS DECODER (Simulates 74HC138) ---
typedef enum {
    TARGET_ROM,
    TARGET_RAM,
    TARGET_INVALID
} MemoryTarget;

MemoryTarget address_decoder(uint32_t address) {
    if (address >= ROM_BASE_ADDR && address <= ROM_END_ADDR) {
        return TARGET_ROM;
    } else if (address >= RAM_BASE_ADDR && address <= RAM_END_ADDR) {
        return TARGET_RAM;
    }
    return TARGET_INVALID;
}

// --- MEMORY BUS CONTROLLER (Read / Write Interface) ---

// Memory Read (Works for both RAM and ROM)
uint8_t memory_read_byte(uint32_t address) {
    MemoryTarget target = address_decoder(address);

    switch (target) {
        case TARGET_ROM:
            return sys_mem.rom[address - ROM_BASE_ADDR];
        
        case TARGET_RAM:
            return sys_mem.ram[address - RAM_BASE_ADDR];

        default:
            printf("[BUS ERROR] Invalid Read Access at Address: 0x%08X\n", address);
            return 0xFF;
    }
}

// Memory Write (Simulates hardware MPU protection)
bool memory_write_byte(uint32_t address, uint8_t data) {
    MemoryTarget target = address_decoder(address);

    switch (target) {
        case TARGET_ROM:
            // ROM Hardware Fault Trigger
            printf("[HARDWARE FAULT] Write violation to ROM at Address: 0x%08X!\n", address);
            return false;

        case TARGET_RAM:
            sys_mem.ram[address - RAM_BASE_ADDR] = data;
            return true;

        default:
            printf("[BUS ERROR] Invalid Write Access at Address: 0x%08X\n", address);
            return false;
    }
}

// --- SYSTEM INITIALIZATION & STACK OPERATIONS ---

void memory_system_init(void) {
    memset(sys_mem.rom, 0xFF, ROM_SIZE); // Flash default state (0xFF)
    memset(sys_mem.ram, 0x00, RAM_SIZE); // RAM initialized to 0x00

    // Initialize Stack Pointer at top of RAM (Full Descending Stack)
    sys_mem.stack_pointer = RAM_END_ADDR;
    sys_mem.stack_limit = RAM_END_ADDR - STACK_SIZE;

    printf("--- Memory Controller Initialized ---\n");
    printf("ROM Range : 0x%08X - 0x%08X (%d KB)\n", ROM_BASE_ADDR, ROM_END_ADDR, ROM_SIZE / 1024);
    printf("RAM Range : 0x%08X - 0x%08X (%d KB)\n", RAM_BASE_ADDR, RAM_END_ADDR, RAM_SIZE / 1024);
    printf("Stack SP  : 0x%08X (Limit: 0x%08X)\n\n", sys_mem.stack_pointer, sys_mem.stack_limit);
}

// Push to Stack with MPU Guarding
bool stack_push(uint8_t data) {
    if (sys_mem.stack_pointer <= sys_mem.stack_limit) {
        printf("[MPU FAULT] Stack Overflow detected at SP: 0x%08X!\n", sys_mem.stack_pointer);
        return false;
    }
    
    memory_write_byte(sys_mem.stack_pointer, data);
    sys_mem.stack_pointer--;
    return true;
}

// Pop from Stack
uint8_t stack_pop(void) {
    if (sys_mem.stack_pointer >= RAM_END_ADDR) {
        printf("[MPU FAULT] Stack Underflow detected!\n");
        return 0x00;
    }
    
    sys_mem.stack_pointer++;
    return memory_read_byte(sys_mem.stack_pointer);
}

// --- MAIN SIMULATION DISPATCHER ---
int main(void) {
    memory_system_init();

    // 1. Flash Bootloader Firmware into ROM
    const char *bootloader_code = "BOOTLOADER_V1.0";
    uint32_t rom_addr = ROM_BASE_ADDR;
    for (int i = 0; bootloader_code[i] != '\0'; i++) {
        sys_mem.rom[rom_addr++ - ROM_BASE_ADDR] = bootloader_code[i];
    }

    // Read ROM Program Memory
    printf("[EXEC] Reading ROM Bootloader String: ");
    for (uint32_t i = ROM_BASE_ADDR; i < ROM_BASE_ADDR + strlen(bootloader_code); i++) {
        putchar(memory_read_byte(i));
    }
    printf("\n\n");

    // 2. Perform RAM Write/Read Operation
    uint32_t sample_ram_addr = RAM_BASE_ADDR + 0x100;
    printf("[RAM TEST] Writing '0x42' to RAM Address: 0x%08X\n", sample_ram_addr);
    memory_write_byte(sample_ram_addr, 0x42);
    printf("[RAM TEST] Read back from RAM: 0x%02X\n\n", memory_read_byte(sample_ram_addr));

    // 3. Attempt Illegal Write to ROM (ROM Guard Test)
    printf("[SECURITY TEST] Attempting illegal write to ROM...\n");
    memory_write_byte(ROM_BASE_ADDR + 0x04, 0x99);
    printf("\n");

    // 4. Test Stack Operations
    printf("[STACK TEST] Pushing values onto Stack...\n");
    stack_push(0xAA);
    stack_push(0xBB);
    printf("[STACK TEST] Popped: 0x%02X\n", stack_pop());
    printf("[STACK TEST] Popped: 0x%02X\n", stack_pop());

    return 0;
}