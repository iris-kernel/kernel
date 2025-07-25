#ifndef BOOTINFO_H
#define BOOTINFO_H

#include <stdint.h>
#include <stddef.h>

#define MEM_REGIONS_MAX 8
#define MEM_RESERVED_MAX 8
#define SYSCON_MAX 4

typedef struct 
{
    uintptr_t base;
    size_t size;
}
mem_region_t;

typedef struct 
{
    uintptr_t base;
    size_t size;
    char name[32];  // Store the node name for identification
}
syscon_device_t;

typedef struct 
{
    int core_count;

    mem_region_t memory_regions[MEM_REGIONS_MAX];
    mem_region_t reserved_regions[MEM_RESERVED_MAX];
    syscon_device_t syscon_devices[SYSCON_MAX];

    int memory_region_count;
    int reserved_region_count;
    int syscon_device_count;

    uintptr_t dtb_base;
    size_t dtb_size;
}
boot_info_t;

typedef void (*kernel_entry_t)(boot_info_t*);

#endif