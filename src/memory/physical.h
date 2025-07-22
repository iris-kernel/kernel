#ifndef PHYSICAL_H
#define PHYSICAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "../bootinfo.h"

#define PAGE_SIZE 4096

#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define ALIGN_UP(addr, align)   (((addr) + (align) - 1) & ~((align) - 1))
#define IS_ALIGNED(addr, align) (((addr) & ((align) - 1)) == 0)

typedef struct
{
    mem_region_t metadata;
    mem_region_t usable;
    size_t page_count;
}
pmm_state_t;

bool phys_init(boot_info_t* info);
void phys_reserve(void* ptr, size_t size);
void* phys_alloc(size_t size);
void phys_free(void* ptr);

#endif // PHYSICAL_H