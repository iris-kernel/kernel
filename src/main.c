#include "bootinfo.h"
#include "dtb/dtb.h"

// Simple UART output for debugging (assuming standard QEMU UART at 0x10000000)
#define UART_BASE 0x10000000
static volatile char* uart = (volatile char*)UART_BASE;

void uart_putc(char c) {
    *uart = c;
}

void uart_puts(const char* str) {
    while (*str) {
        uart_putc(*str++);
    }
}

void uart_putx(uint64_t val) {
    uart_puts("0x");
    for (int i = 15; i >= 0; i--) {
        int digit = (val >> (i * 4)) & 0xf;
        uart_putc(digit < 10 ? '0' + digit : 'a' + digit - 10);
    }
}

// Placeholder kernel main function
void kmain(boot_info_t* info) {
    uart_puts("RISC-V Kernel Starting!\n");
    uart_puts("Core count: ");
    uart_putx(info->core_count);
    uart_puts("\n");
    
    uart_puts("Memory regions:\n");
    for (int i = 0; i < info->memory_region_count; i++) {
        uart_puts("  Region ");
        uart_putx(i);
        uart_puts(": base=");
        uart_putx(info->memory_regions[i].base);
        uart_puts(" size=");
        uart_putx(info->memory_regions[i].size);
        uart_puts("\n");
    }
    
    uart_puts("Kernel initialization complete!\n");
    
    // Halt
    while (1) {
        asm volatile("wfi");
    }
}

void boot_cmain(const void* dtb_ptr) {
    uart_puts("Bootloader starting...\n");
    uart_puts("DTB pointer: ");
    uart_putx((uint64_t)dtb_ptr);
    uart_puts("\n");

    boot_info_t info;
    
    // Initialize the boot info structure
    info.core_count = 0;
    info.memory_region_count = 0;

    // Check DTB magic number first
    if (dtb_ptr) {
        uint32_t magic_raw = *(uint32_t*)dtb_ptr;
        uint32_t magic = ((magic_raw & 0xFF000000) >> 24) |
                        ((magic_raw & 0x00FF0000) >> 8)  |
                        ((magic_raw & 0x0000FF00) << 8)  |
                        ((magic_raw & 0x000000FF) << 24);
        uart_puts("DTB magic: ");
        uart_putx(magic);
        uart_puts(" (should be 0xd00dfeed)\n");
    }

    dtb_parse(dtb_ptr, &info);

    uart_puts("DTB parsed successfully\n");
    uart_puts("Parsed core count: ");
    uart_putx(info.core_count);
    uart_puts("\n");
    uart_puts("Parsed memory regions: ");
    uart_putx(info.memory_region_count);
    uart_puts("\n");
    
    kmain(&info);
}