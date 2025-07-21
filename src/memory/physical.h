#ifndef PHYSICAL_H
#define PHYSICAL_H

#include <stdbool.h>
#include <stdint.h>

#include "../bootinfo.h"

#define MAX_USABLE_REGIONS 8

typedef struct
{
    mem_region_t metadata;
    mem_region_t usable;
}
pmm_state_t;

bool phys_init(boot_info_t* info);
void* phys_alloc();
void phys_free(void* ptr);

#endif // PHYSICAL_H