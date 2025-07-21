#ifndef BOOTINFO_H
#define BOOTINFO_H

#include <stdint.h>
#include <stddef.h>

#define MEM_REGIONS_MAX 8
#define MEM_RESERVED_MAX 8

typedef struct 
{
    int core_count;

    struct 
    {
        uintptr_t base;
        size_t size;
    }
    memory_regions[MEM_REGIONS_MAX];

    struct 
    {
        uintptr_t base;
        size_t size;
    }
    reserved_regions[MEM_RESERVED_MAX];

    int memory_region_count;
    int reserved_region_count;

    uintptr_t dtb_base;
    size_t dtb_size;
}
boot_info_t;

typedef void (*kernel_entry_t)(boot_info_t*);

#endif