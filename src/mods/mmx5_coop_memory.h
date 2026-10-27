#ifndef MMX5_COOP_MEMORY_H
#define MMX5_COOP_MEMORY_H
#include <stddef.h>
#include <stdint.h>
/* Co-op RAM/scratchpad/mod-arena state only: never use on device registers.
 * Word stores still use the runtime's host store path and its invalidation. */
void mmx5_coop_capture(uint32_t address,void *data,size_t bytes);
void mmx5_coop_project(uint32_t address,const void *data,size_t bytes);
#endif
