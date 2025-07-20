#ifndef DTB_PARSER_H
#define DTB_PARSER_H

#include "../bootinfo.h"

void dtb_parse(const void* dtb_ptr, boot_info_t* out);

#endif