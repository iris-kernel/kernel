#include "dtb.h"

#define FDT_MAGIC       0xd00dfeed
#define FDT_BEGIN_NODE  0x1
#define FDT_END_NODE    0x2
#define FDT_PROP        0x3
#define FDT_NOP         0x4
#define FDT_END         0x9

typedef struct 
{
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
}
fdt_header_t;

static uint32_t fdt32_to_cpu(uint32_t x) 
{
    return __builtin_bswap32(x);
}

static const char* fdt_get_string(const void* dtb, uint32_t offset) 
{
    const fdt_header_t* hdr = (const fdt_header_t*)dtb;
    return (const char*)dtb + fdt32_to_cpu(hdr->off_dt_strings) + offset;
}

void dtb_parse(const void* dtb_ptr, boot_info_t* out)
{
    const fdt_header_t* hdr = (const fdt_header_t*)dtb_ptr;

    if (fdt32_to_cpu(hdr->magic) != FDT_MAGIC) return;

    const char* struct_block = (const char*)dtb_ptr + fdt32_to_cpu(hdr->off_dt_struct);
    const char* ptr = struct_block;
    const char* strings_block = (const char*)dtb_ptr + fdt32_to_cpu(hdr->off_dt_strings);

    const char* current_node = NULL;
    int depth = 0;
    int in_cpus = 0;
    int in_memory = 0;
    out->core_count = 0;
    out->memory_region_count = 0;

    while (1) 
    {
        uint32_t token = fdt32_to_cpu(*(uint32_t*)ptr);
        ptr += 4;

        switch (token) 
        {
            case FDT_BEGIN_NODE: 
            {
                const char* name = ptr;
                size_t len = strlen(name);
                
                if (strcmp(name, "cpus") == 0 && depth == 0)
                    in_cpus = 1;
                else if (in_cpus && depth == 1)
                    out->core_count++;
                
                if (strcmp(name, "memory") == 0 && depth == 0)
                    in_memory = 1;

                ptr += len + 1;
                ptr = (const char*)(((uintptr_t)ptr + 3) & ~3); // Align to 4
                depth++;
                break;
            }
            case FDT_END_NODE:
                depth--;
                if (depth == 0) 
                {
                    in_cpus = 0;
                    in_memory = 0;
                }
                break;
            case FDT_PROP: 
            {
                uint32_t len = fdt32_to_cpu(*(uint32_t*)ptr); ptr += 4;
                uint32_t nameoff = fdt32_to_cpu(*(uint32_t*)ptr); ptr += 4;
                const char* name = fdt_get_string(dtb_ptr, nameoff);
                const void* value = ptr;

                if (in_memory && strcmp(name, "reg") == 0 && len >= 16) 
                {
                    const uint64_t* data = (const uint64_t*)value;
                    out->memory_regions[0].base = __builtin_bswap64(data[0]);
                    out->memory_regions[0].size = __builtin_bswap64(data[1]);
                    out->memory_region_count = 1;
                }

                ptr += (len + 3) & ~3; // Align
                break;
            }
            case FDT_NOP:
                break;
            case FDT_END:
                return;
            default:
                return;
        }
    }
}