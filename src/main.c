#include "bootinfo.h"
#include "device/dtb.h"
#include "memory/physical.h"
#include "device/opensbi.h"

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

        sbi_shutdown();

        // Halt
        while (1) 
        {
            asm volatile("wfi");
        }
    }
    
    sbi_shutdown();

    while (1) 
    {
        asm volatile("wfi");
    }
}

void boot_cmain(const void* dtb_ptr) 
{
    boot_info_t info;
    info.core_count = 0;
    info.memory_region_count = 0;

    dtb_parse(dtb_ptr, &info);
    
    kmain(&info);
}