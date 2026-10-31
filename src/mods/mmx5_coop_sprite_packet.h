#ifndef MMX5_COOP_SPRITE_PACKET_H
#define MMX5_COOP_SPRITE_PACKET_H

#include <stdint.h>

/* Native 80024C60 reserves ten words for either an FT4 or a SPRT_16.
 * Unflipped tiles use SPRT_16; flipped tiles use FT4. Preserve FT4 geometry
 * exactly, and express SPRT_16 with tile-local UVs into a retained 16x16 bank.
 * This also avoids wrapping an endpoint of 256 into the next texture row. */
static inline int mmx5_coop_sprite_quad(uint32_t q[10], uint16_t *tile) {
    unsigned command=q[1]>>24;
    *tile=UINT16_MAX;
    if ((command&0xFC)==0x2C) return 1;
    if ((command&0xFC)!=0x7C) return 0;
    int x=(int16_t)q[2], y=(int16_t)(q[2]>>16);
    uint32_t clut=q[3]&0xFFFF0000u;
    *tile=(uint16_t)q[3];
    q[1]=(q[1]&0x03FFFFFFu)|0x2C000000u;
    for (unsigned i=0;i<4;++i) {
        q[2+i*2]=(uint16_t)(x+(i&1)*16)|((uint32_t)(uint16_t)(y+(i>>1)*16)<<16);
        q[3+i*2]=(i&1)*16|((i>>1)*16<<8);
    }
    q[3]|=clut;
    return 1;
}

/* Compressed assembly tiles use a private origin. Original 80024FB8..24FCC
 * and 80025038..2504C clip an edge when its flipped UV would be -1.
 * Retain the authored sixteen-pixel square and reflect the source pixels,
 * as in X4's native-page-edge fix. Low key bits encode X/Y reflection;
 * genuine native tile origins are aligned to sixteen pixels. Apply only
 * to compressed FT4s and retained tile-aligned character image FT4s.
 * Other native packets keep their original painter and UVs. */
static inline uint16_t mmx5_coop_sprite_quad_tile(uint32_t q[10]) {
    unsigned u=q[3]&255u,v=(q[3]>>8)&255u;
    unsigned flip_x=u>(q[5]&255u),flip_y=v>((q[7]>>8)&255u);
    int x=(int16_t)q[2],y=(int16_t)(q[2]>>16);
    uint16_t tile=(uint16_t)((u&0xF0u)|((v&0xF0u)<<8)|flip_x|(flip_y<<1));
    for(unsigned i=0;i<4;++i) {
        q[2+i*2]=(uint16_t)(x+(i&1)*16)|((uint32_t)(uint16_t)(y+(i>>1)*16)<<16);
        q[3+i*2]=(q[3+i*2]&0xFFFF0000u)|(i&1)*16|((i>>1)*16<<8);
    }
    return tile;
}
static inline void mmx5_coop_sprite_tile_pixels(const uint16_t pixels[256*256],
                                               uint16_t tile,uint16_t out[16*16]) {
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x) {
        unsigned sx=(tile&0xF0u)+((tile&1u)?15-x:x);
        unsigned sy=((tile>>8)&0xF0u)+((tile&2u)?15-y:y);
        out[y*16+x]=pixels[sy*256+sx];
    }
}
#endif
