#include "physical.h"

#include <stddef.h>
#include <stdint.h>

static void get_overlap(mem_region_t* out, mem_region_t* a, mem_region_t* b)
{
    uintptr_t al = a->base + a->size;
    uintptr_t bl = b->base + b->size;
    out->base = a->base > b->base ? a->base : b->base;
    out->size = al < bl ? al : bl;
    out->size -= out->base;

    if ((intptr_t)out->size < 0)
        out->size = 0;
}

bool phys_init(boot_info_t* info)
{
    size_t available = 0;

    for(int i = 0; i < info->memory_region_count; ++i)
    {
        mem_region_t* region = &info->memory_regions[i];
        available += region->size;

        for(int j = 0; j < info->reserved_region_count; ++j)
        {
            mem_region_t* reserved = &info->reserved_regions[j];

            mem_region_t overlap;
            get_overlap(&overlap, region, reserved);
            available -= overlap.size;
        }
    }

    uart_puts("Available Memory: ");
    uart_puti(available / (1024 * 1024));
    uart_puts(" MiB\n");

    return true;
}

void phys_reserve(void* ptr)
{

}

void* phys_alloc()
{
    return 0;
}

void phys_free(void* ptr)
{

}