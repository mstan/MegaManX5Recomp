#include "mmx5_coop_texture_cache.h"
#include <limits.h>
#include <string.h>

static void retain_packets(uint8_t used[512],uint16_t first,uint32_t begin,
    unsigned bytes,uint32_t (*read_word)(uint32_t)) {
    for(unsigned offset=0;offset+40<=bytes;offset+=40) {
        uint32_t at=begin+offset;
        if((read_word(at)>>24)!=9 || (read_word(at+4)&0xFC000000u)!=0x34000000u)continue;
        unsigned id=(read_word(at+16)>>24)|((read_word(at+28)>>24)<<8);
        if(id>=first && id-first<4096)used[(id-first)/8]|=(uint8_t)(1u<<((id-first)%8));
    }
}
unsigned mmx5_coop_texture_cache_slot(unsigned count,uint16_t first,
    unsigned start,uint32_t world,uint32_t primary_world,uint32_t hud,
    uint32_t (*read_word)(uint32_t)) {
    if(count<4096)return count;
    if(count!=4096 || !world || !primary_world || !hud || !read_word)return UINT_MAX;
    uint8_t used[512];memset(used,0,sizeof used);
    for(unsigned buffer=0;buffer<2;++buffer) {
        /* render_actor expands native40-byte packets into pairs after the
         * 40,000-byte scratch region, at most80,000 bytes. Pin old tails too.
         * X5 has separate P1/P2 world arenas: omitting either can corrupt live
         * art even if the other world and both HUD buffers were scanned. */
        retain_packets(used,first,world+buffer*0x40000u+40000u,80000u,read_word);
        retain_packets(used,first,primary_world+buffer*0x40000u+40000u,80000u,read_word);
        retain_packets(used,first,hud+buffer*0x8000u+0x3000u,0x5000u,read_word);
    }
    for(unsigned i=0;i<4096;++i) {
        unsigned slot=(start+i)&4095u;
        if(!(used[slot/8]&(1u<<(slot%8))))return slot;
    }
    /* Six buffers can conservatively pin the whole pool. Refuse replacement
     * rather than changing artwork used by a current or previous packet. */
    return UINT_MAX;
}
