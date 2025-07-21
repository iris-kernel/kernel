#include "physical.h"

#include <stddef.h>
#include <stdint.h>

static void get_overlap(mem_region_t* out, mem_region_t* a, mem_region_t* b);
static mem_region_t find_largest_gap(mem_region_t* available, int avail_count, mem_region_t* reserved, int reserved_count);

static pmm_state_t pmm_state;

bool phys_init(boot_info_t* info)
{
    mem_region_t best_region = find_largest_gap
    (
        info->memory_regions,
        info->memory_region_count,
        info->reserved_regions,
        info->reserved_region_count
    );

    pmm_state.metadata.base = best_region.base;

    uart_puts("Available Memory: ");
    uart_puti(best_region.size / (1024 * 1024));
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

static mem_region_t find_largest_gap(mem_region_t* available, int avail_count, mem_region_t* reserved, int reserved_count) 
{
    mem_region_t best = {0};
    uint64_t best_size = 0;

    for (int i = 0; i < avail_count; ++i) 
    {
        mem_region_t region = available[i];

        // Collect reserved regions that fall within this available block
        // and sort them by base
        mem_region_t contained[MEM_RESERVED_MAX];
        int count = 0;

        for (int j = 0; j < reserved_count; ++j) 
        {
            uint64_t r_start = reserved[j].base;
            uint64_t r_end = r_start + reserved[j].size;
            uint64_t a_start = region.base;
            uint64_t a_end = a_start + region.size;

            if (r_end > a_start && r_start < a_end)
            {
                contained[count++] = reserved[j];
            }
        }

        // Sort by base
        for (int x = 0; x < count - 1; ++x) 
        {
            for (int y = x + 1; y < count; ++y) 
            {
                if (contained[x].base > contained[y].base) 
                {
                    mem_region_t tmp = contained[x];
                    contained[x] = contained[y];
                    contained[y] = tmp;
                }
            }
        }

        // Now scan gaps between reservations
        uint64_t last = region.base;

        for (int k = 0; k <= count; ++k) 
        {
            uint64_t next = (k == count)
                ? (region.base + region.size)
                : contained[k].base;

            if (next > last) 
            {
                uint64_t gap_size = next - last;
                if (gap_size > best_size) 
                {
                    best.base = last;
                    best.size = gap_size;
                    best_size = gap_size;
                }
            }

            if (k < count) 
            {
                uint64_t r_end = contained[k].base + contained[k].size;
                if (r_end > last)
                    last = r_end;
            }
        }
    }

    return best;
}