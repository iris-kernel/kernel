#ifndef PHYSICAL_H
#define PHYSICAL_H

#include <stdbool.h>

#include "../bootinfo.h"

bool phys_init(boot_info_t* info);
void* phys_alloc();
void phys_free(void* ptr);

#endif // PHYSICAL_H