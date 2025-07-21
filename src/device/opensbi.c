#include "opensbi.h"

#define SBI_ECALL_HFENCE_GVMA    0   // internal, ignore
#define SBI_ECALL_SHUTDOWN       8
#define SBI_ECALL_SYSTEM_RESET   2

#include <stdint.h>

void sbi_shutdown(void) 
{
    register uintptr_t a7 asm("a7") = SBI_ECALL_SHUTDOWN;
    register uintptr_t a0 asm("a0") = 0;  // unused

    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a7)
                 : "memory");
    for (;;);
}

void sbi_reboot(void) 
{
    register uintptr_t a0 asm("a0") = 0;
    register uintptr_t a1 asm("a1") = 0;
    register uintptr_t a7 asm("a7") = SBI_ECALL_SYSTEM_RESET;

    asm volatile("ecall"
                 : "+r"(a0), "+r"(a1)
                 : "r"(a7)
                 : "memory");
    for (;;);
}