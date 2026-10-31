#ifndef MMX5_COOP_TEXTURE_CACHE_H
#define MMX5_COOP_TEXTURE_CACHE_H
#include <stdint.h>

/* Pin IDs referenced by both buffers of BOTH X5 world packet arenas and HUD.
 * Reuse depends only on guest packets and a guest clock, never a host LRU. */
unsigned mmx5_coop_texture_cache_slot(unsigned count,uint16_t first,
    unsigned start,uint32_t world,uint32_t primary_world,uint32_t hud,
    uint32_t (*read_word)(uint32_t));
#endif
