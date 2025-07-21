#include "dtb.h"

// External UART functions for debugging
extern void uart_puts(const char* str);
extern void uart_putx(uint64_t val);

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

// Simple string functions since we're in freestanding environment
static int my_strlen(const char* str) 
{
    int len = 0;
    while (str[len]) len++;
    return len;
}

static int my_strcmp(const char* a, const char* b) 
{
    while (*a && (*a == *b)) 
    {
        a++;
        b++;
    }
    return *(unsigned char*)a - *(unsigned char*)b;
}

static int my_strncmp(const char* a, const char* b, int n) 
{
    for (int i = 0; i < n; i++) 
    {
        if (a[i] != b[i] || a[i] == '\0') 
        {
            return a[i] - b[i];
        }
    }
    return 0;
}

static uint32_t fdt32_to_cpu(uint32_t x) 
{
    return ((x & 0xFF000000) >> 24) |
           ((x & 0x00FF0000) >> 8)  |
           ((x & 0x0000FF00) << 8)  |
           ((x & 0x000000FF) << 24);
}

static uint64_t fdt64_to_cpu(uint64_t x) 
{
    return ((x & 0xFF00000000000000ULL) >> 56) |
           ((x & 0x00FF000000000000ULL) >> 40) |
           ((x & 0x0000FF0000000000ULL) >> 24) |
           ((x & 0x000000FF00000000ULL) >> 8)  |
           ((x & 0x00000000FF000000ULL) << 8)  |
           ((x & 0x0000000000FF0000ULL) << 24) |
           ((x & 0x000000000000FF00ULL) << 40) |
           ((x & 0x00000000000000FFULL) << 56);
}

static const char* fdt_get_string(const void* dtb, uint32_t offset) 
{
    const fdt_header_t* hdr = (const fdt_header_t*)dtb;
    return (const char*)dtb + fdt32_to_cpu(hdr->off_dt_strings) + offset;
}

void dtb_parse(const void* dtb_ptr, boot_info_t* out)
{
    if (!dtb_ptr || !out) return;
    
    const fdt_header_t* hdr = (const fdt_header_t*)dtb_ptr;

    if (fdt32_to_cpu(hdr->magic) != FDT_MAGIC) return;

    const char* struct_block = (const char*)dtb_ptr + fdt32_to_cpu(hdr->off_dt_struct);
    const char* ptr = struct_block;

    int depth = 0;
    int in_cpus = 0;
    int in_memory = 0;
    int in_cpu_node = 0;
    out->core_count = 0;
    out->memory_region_count = 0;

    out->dtb_base = dtb_ptr;
    out->dtb_size = fdt32_to_cpu(hdr->totalsize);

    while (1) 
    {
        uint32_t token = fdt32_to_cpu(*(uint32_t*)ptr);
        ptr += 4;

        switch (token) 
        {
            case FDT_BEGIN_NODE: 
            {
                const char* name = ptr;
                int len = my_strlen(name);
            
                // Check nodes at the correct depth BEFORE incrementing
                // Root node is at depth 0, its children (cpus, memory) are at depth 1
                if (my_strcmp(name, "cpus") == 0 && depth == 1) 
                {
                    in_cpus = 1;
                }
                else if (in_cpus && depth == 2 && my_strncmp(name, "cpu@", 4) == 0) 
                {
                    in_cpu_node = 1;
                    out->core_count++;
                }
                
                if ((my_strcmp(name, "memory") == 0 || my_strncmp(name, "memory@", 7) == 0) && depth == 1) 
                {
                    in_memory = 1;
                }

                ptr += len + 1;
                ptr = (const char*)(((uintptr_t)ptr + 3) & ~3); // Align to 4
                depth++; // Increment depth AFTER processing the node
                break;
            }
            case FDT_END_NODE:
                depth--;
                if (depth == 1) 
                {
                    in_cpus = 0;
                    in_memory = 0;
                }
                if (depth == 2 && in_cpus) 
                {
                    in_cpu_node = 0;
                }
                break;
            case FDT_PROP: 
            {
                uint32_t prop_len = fdt32_to_cpu(*(uint32_t*)ptr); ptr += 4;
                uint32_t nameoff = fdt32_to_cpu(*(uint32_t*)ptr); ptr += 4;
                const char* prop_name = fdt_get_string(dtb_ptr, nameoff);
                const void* value = ptr;

                // Parse memory regions
                if (in_memory && my_strcmp(prop_name, "reg") == 0 && 
                    out->memory_region_count < MEM_REGIONS_MAX) 
                {
                    // Memory reg properties are typically arrays of (address, size) pairs
                    // Each pair can be 32-bit or 64-bit values
                    // Try 64-bit first (16 bytes per entry)
                    if (prop_len >= 16 && (prop_len % 16) == 0) 
                    {
                        const uint64_t* data = (const uint64_t*)value;
                        int entries = prop_len / 16;
                        for (int i = 0; i < entries && out->memory_region_count < MEM_REGIONS_MAX; i++) 
                        {
                            out->memory_regions[out->memory_region_count].base = fdt64_to_cpu(data[i*2]);
                            out->memory_regions[out->memory_region_count].size = fdt64_to_cpu(data[i*2+1]);
                            out->memory_region_count++;
                        }
                    } 
                    // Try 32-bit (8 bytes per entry)
                    else if (prop_len >= 8 && (prop_len % 8) == 0) 
                    {
                        const uint32_t* data32 = (const uint32_t*)value;
                        int entries = prop_len / 8;
                        for (int i = 0; i < entries && out->memory_region_count < MEM_REGIONS_MAX; i++) 
                        {
                            out->memory_regions[out->memory_region_count].base = fdt32_to_cpu(data32[i*2]);
                            out->memory_regions[out->memory_region_count].size = fdt32_to_cpu(data32[i*2+1]);
                            out->memory_region_count++;
                        }
                    }
                    else 
                    {
                        uart_puts("    Unexpected reg property length\n");
                    }
                }

                ptr += (prop_len + 3) & ~3; // Align to 4-byte boundary
                break;
            }
            case FDT_NOP:
                break;
            case FDT_END:
                return;
            default:
                uart_puts("Unknown DTB token: ");
                uart_putx(token);
                uart_puts("\n");
                return; // Unknown token
        }
    }
}