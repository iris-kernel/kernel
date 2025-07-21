#include "bootinfo.h"
#include "dtb/dtb.h"
#include "memory/physical.h"

// Simple UART output for debugging (assuming standard QEMU UART at 0x10000000)
#define UART_BASE 0x10000000
static volatile char* uart = (volatile char*)UART_BASE;

void uart_putc(char c) 
{
    *uart = c;
}

void uart_puts(const char* str) 
{
    while (*str) 
    {
        uart_putc(*str++);
    }
}

void uart_putx(uint64_t val) 
{
    uart_puts("0x");
    for (int i = 15; i >= 0; i--) 
    {
        int digit = (val >> (i * 4)) & 0xf;
        uart_putc(digit < 10 ? '0' + digit : 'A' + digit - 10);
    }
}

void uart_puti(int n) {
    char buf[12];
    int i = 0;
    unsigned int un;

    if (n < 0) 
    {
        uart_putc('-');
        un = (unsigned int)(-(n + 1)) + 1;
    } else {
        un = (unsigned int)n;
    }
    
    if (un == 0) 
    {
        uart_putc('0');
        return;
    }
    
    while (un > 0) 
    {
        buf[i++] = '0' + (un % 10);
        un /= 10;
    }
    
    while (i > 0) 
    {
        uart_putc(buf[--i]);
    }
}

void kmain(boot_info_t* info) 
{
    uart_puts("Iris Kernel pre-rel. 0.0.1\n");
    uart_puts("Core count: ");
    uart_puti(info->core_count);
    uart_puts("\n");
    
    if(!phys_init(info))
    {
        uart_puts("ERROR: Failed to Initialize PMM!\n");

        // Halt
        while (1) 
        {
            asm volatile("wfi");
        }
    }
    
    // Halt
    while (1) 
    {
        asm volatile("wfi");
    }
}

void boot_cmain(const void* dtb_ptr) 
{
    boot_info_t info;
    
    // Initialize the boot info structure
    info.core_count = 0;
    info.memory_region_count = 0;

    // Check DTB magic number first
    /*
    if (dtb_ptr) 
    {
        uint32_t magic_raw = *(uint32_t*)dtb_ptr;
        uint32_t magic = ((magic_raw & 0xFF000000) >> 24) |
                        ((magic_raw & 0x00FF0000) >> 8)  |
                        ((magic_raw & 0x0000FF00) << 8)  |
                        ((magic_raw & 0x000000FF) << 24);
        uart_puts("DTB magic: ");
        uart_putx(magic);
        uart_puts(" (should be 0xd00dfeed)\n");
    }
    */
    
    dtb_parse(dtb_ptr, &info);
    
    kmain(&info);
}