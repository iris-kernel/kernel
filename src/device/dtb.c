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

typedef struct 
{
    uint64_t address;
    uint64_t size;
}
fdt_reserve_entry_t;

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

static void my_strncpy(char* dest, const char* src, int n) 
{
    int i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) 
    {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

// Check if a string contains a substring
static int my_strstr(const char* haystack, const char* needle) 
{
    int needle_len = my_strlen(needle);
    int haystack_len = my_strlen(haystack);
    
    if (needle_len > haystack_len) return 0;
    
    for (int i = 0; i <= haystack_len - needle_len; i++) 
    {
        if (my_strncmp(&haystack[i], needle, needle_len) == 0) 
        {
            return 1;
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

static void parse_reserved_memory_map(const void* dtb_ptr, boot_info_t* out)
{
    const fdt_header_t* hdr = (const fdt_header_t*)dtb_ptr;
    const fdt_reserve_entry_t* rsvmap = (const fdt_reserve_entry_t*)
        ((const char*)dtb_ptr + fdt32_to_cpu(hdr->off_mem_rsvmap));

    out->reserved_region_count = 0;
    
    // Parse the reserved memory map entries
    for (int i = 0; i < MEM_RESERVED_MAX; i++) 
    {
        uint64_t address = fdt64_to_cpu(rsvmap[i].address);
        uint64_t size = fdt64_to_cpu(rsvmap[i].size);
        
        // End of reserved memory map is marked by entry with address=0 and size=0
        if (address == 0 && size == 0) 
            break;
            
        out->reserved_regions[out->reserved_region_count].base = address;
        out->reserved_regions[out->reserved_region_count].size = size;
        out->reserved_region_count++;
    }
}

static void parse_reserved_memory_nodes(const void* dtb_ptr, boot_info_t* out,
                                        const char* struct_block)
{
    const char* ptr = struct_block;
    int depth = 0;
    int in_reserved_memory = 0;
    int in_reserved_child = 0;
    
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
                
                // Look for reserved-memory node at depth 1
                if (my_strcmp(name, "reserved-memory") == 0 && depth == 1) 
                {
                    in_reserved_memory = 1;
                }
                // Look for child nodes of reserved-memory at depth 2
                else if (in_reserved_memory && depth == 2) 
                {
                    in_reserved_child = 1;
                }

                ptr += len + 1;
                ptr = (const char*)(((uintptr_t)ptr + 3) & ~3); // Align to 4
                depth++;
                break;
            }
            case FDT_END_NODE:
                depth--;
                if (depth == 1) 
                {
                    in_reserved_memory = 0;
                }
                if (depth == 2 && in_reserved_memory) 
                {
                    in_reserved_child = 0;
                }
                break;
            case FDT_PROP: 
            {
                uint32_t prop_len = fdt32_to_cpu(*(uint32_t*)ptr); ptr += 4;
                uint32_t nameoff = fdt32_to_cpu(*(uint32_t*)ptr); ptr += 4;
                const char* prop_name = fdt_get_string(dtb_ptr, nameoff);
                const void* value = ptr;

                // Parse reg properties in reserved-memory child nodes
                if (in_reserved_child && my_strcmp(prop_name, "reg") == 0 && 
                    out->reserved_region_count < MEM_RESERVED_MAX) 
                {
                    // Similar to memory parsing - try 64-bit first
                    if (prop_len >= 16 && (prop_len % 16) == 0) 
                    {
                        const uint64_t* data = (const uint64_t*)value;
                        int entries = prop_len / 16;
                        for (int i = 0; i < entries && out->reserved_region_count < MEM_RESERVED_MAX; i++) 
                        {
                            out->reserved_regions[out->reserved_region_count].base = fdt64_to_cpu(data[i*2]);
                            out->reserved_regions[out->reserved_region_count].size = fdt64_to_cpu(data[i*2+1]);
                            out->reserved_region_count++;
                        }
                    } 
                    // Try 32-bit
                    else if (prop_len >= 8 && (prop_len % 8) == 0) 
                    {
                        const uint32_t* data32 = (const uint32_t*)value;
                        int entries = prop_len / 8;
                        for (int i = 0; i < entries && out->reserved_region_count < MEM_RESERVED_MAX; i++) 
                        {
                            out->reserved_regions[out->reserved_region_count].base = fdt32_to_cpu(data32[i*2]);
                            out->reserved_regions[out->reserved_region_count].size = fdt32_to_cpu(data32[i*2+1]);
                            out->reserved_region_count++;
                        }
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
                return; // Unknown token
        }
    }
}

void dtb_parse(const void* dtb_ptr, boot_info_t* out)
{
    if (!dtb_ptr || !out) return;
    
    const fdt_header_t* hdr = (const fdt_header_t*)dtb_ptr;

    if (fdt32_to_cpu(hdr->magic) != FDT_MAGIC) return;

    // Initialize counters
    out->core_count = 0;
    out->memory_region_count = 0;
    out->reserved_region_count = 0;
    out->syscon_device_count = 0;

    out->dtb_base = (uintptr_t)dtb_ptr;
    out->dtb_size = fdt32_to_cpu(hdr->totalsize);

    // First, parse the reserved memory map from the header
    parse_reserved_memory_map(dtb_ptr, out);

    const char* struct_block = (const char*)dtb_ptr + fdt32_to_cpu(hdr->off_dt_struct);
    const char* ptr = struct_block;

    int depth = 0;
    int in_cpus = 0;
    int in_memory = 0;
    int in_cpu_node = 0;
    int checking_syscon = 0;
    
    // Store current node name for syscon parsing
    char current_node_name[32] = {0};

    // Parse the device tree structure for memory regions, CPUs, and syscon devices
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
                
                // Store node name for potential syscon device and mark for checking
                if (depth >= 1 && out->syscon_device_count < SYSCON_MAX) 
                {
                    my_strncpy(current_node_name, name, sizeof(current_node_name));
                    checking_syscon = 1;
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
                checking_syscon = 0;
                current_node_name[0] = '\0';
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

                // Check for syscon devices by looking at compatible property
                if (checking_syscon && my_strcmp(prop_name, "compatible") == 0) 
                {
                    const char* compat_str = (const char*)value;
                    
                    // Check if this is a syscon device
                    // Compatible strings are null-terminated and may contain multiple entries
                    int offset = 0;
                    while (offset < (int)prop_len) 
                    {
                        const char* current_compat = compat_str + offset;
                        
                        if (my_strstr(current_compat, "syscon") || 
                            my_strcmp(current_compat, "simple-mfd") == 0 ||
                            my_strstr(current_compat, "sifive,test") || 
                            my_strstr(current_compat, "shutdown"))
                        {
                            // This is a syscon device, now we need to find its reg property
                            // We'll mark it and handle it when we encounter the reg property
                            checking_syscon = 2; // Mark as confirmed syscon
                            break;
                        }
                        
                        offset += my_strlen(current_compat) + 1;
                    }
                }
                
                // Parse syscon device registers
                if (checking_syscon == 2 && my_strcmp(prop_name, "reg") == 0 && 
                    out->syscon_device_count < SYSCON_MAX) 
                {
                    // Parse the first reg entry for the syscon device
                    if (prop_len >= 16 && (prop_len % 16) == 0) 
                    {
                        const uint64_t* data = (const uint64_t*)value;
                        out->syscon_devices[out->syscon_device_count].base = fdt64_to_cpu(data[0]);
                        out->syscon_devices[out->syscon_device_count].size = fdt64_to_cpu(data[1]);
                        my_strncpy(out->syscon_devices[out->syscon_device_count].name, 
                                  current_node_name, sizeof(out->syscon_devices[out->syscon_device_count].name));
                        out->syscon_device_count++;
                    } 
                    else if (prop_len >= 8 && (prop_len % 8) == 0) 
                    {
                        const uint32_t* data32 = (const uint32_t*)value;
                        out->syscon_devices[out->syscon_device_count].base = fdt32_to_cpu(data32[0]);
                        out->syscon_devices[out->syscon_device_count].size = fdt32_to_cpu(data32[1]);
                        my_strncpy(out->syscon_devices[out->syscon_device_count].name, 
                                  current_node_name, sizeof(out->syscon_devices[out->syscon_device_count].name));
                        out->syscon_device_count++;
                    }
                    
                    uart_puts("Found syscon device: ");
                    uart_puts(out->syscon_devices[out->syscon_device_count - 1].name);
                    uart_puts(" @ ");
                    uart_putx(out->syscon_devices[out->syscon_device_count - 1].base);
                    uart_puts("\n");
                    checking_syscon = 0; // Reset after processing
                }

                ptr += (prop_len + 3) & ~3; // Align to 4-byte boundary
                break;
            }
            case FDT_NOP:
                break;
            case FDT_END:
                // After parsing the main structure, parse reserved-memory nodes
                parse_reserved_memory_nodes(dtb_ptr, out, struct_block);
                return;
            default:
                uart_puts("Unknown DTB token: ");
                uart_putx(token);
                uart_puts("\n");
                return; // Unknown token
        }
    }
}