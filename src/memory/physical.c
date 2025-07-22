#include "physical.h"

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

    /* Set page table region */
    uint64_t original_base = best_region.base;
    uint64_t original_size = best_region.size;
    pmm_state.metadata.base = ALIGN_UP(original_base, 64);
    pmm_state.metadata.size = original_size / (PAGE_SIZE * 2); // 4 bits for each min sized page

    /* Set usable region */
    uint64_t metadata_end = pmm_state.metadata.base + pmm_state.metadata.size;
    pmm_state.usable.base = ALIGN_UP(metadata_end, PAGE_SIZE);
    pmm_state.usable.size = (original_base + original_size) - best_region.base;
    pmm_state.page_count = pmm_state.usable.size / PAGE_SIZE;

    /* Null table */
    for(size_t i = 0; i < pmm_state.metadata.size; i += 8)
    {
        uint64_t* pp = pmm_state.metadata.base + i;
        *pp = 0x0000000000000000;
    }

    uart_puts("Available Memory: ");
    uart_puti(best_region.size / (1024 * 1024));
    uart_puts(" MiB\n");

    return true;
}

void phys_reserve(void* ptr, size_t size)
{

}

static uint8_t get_page(uintmax_t index)
{
    int m = index % 2;
    uint8_t* base = pmm_state.metadata.base + index / 2;
    uint8_t meta = 0b1100;

    if(m == 1)
        meta = (*base) & 0x0F;
    else
        meta = ((*base) & 0xF0) >> 4;

    return meta;  
}

void* phys_alloc(size_t size)
{
    bool found = false;
    uintmax_t pages = 0;

    while(!found)
    {
        while(1)
        {
            uint8_t* meta =  
        }
    }

    return 0;
}

void phys_free(void* ptr)
{
    if(ptr < pmm_state.usable.base)
        return;

    if(ptr > pmm_state.page_count * 4096)
        return;
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