#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <stdint.h>

#define MEM_REGIONS_MAX 8

typedef struct 
{
    int core_count;

    struct 
    {
        uint64_t base;
        uint64_t size;
    }
    memory_regions[MEM_REGIONS_MAX];

    int memory_region_count;
}
boot_info_t;

typedef void (*kernel_entry_t)(boot_info_t*);

#endif