#include "bootinfo.h"
#include "dtb/dtb.h"

void boot_cmain(const void* dtb_ptr)
{
    boot_info_t info;

    dtb_parse(dtb_ptr, &info);

    kmain(&info);
}
