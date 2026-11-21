#ifndef MMX5_COOP_BINDINGS_H
#define MMX5_COOP_BINDINGS_H
#include <stdint.h>
#include <string.h>

/* Native cards already serialize all sixteen mapping halfwords. The final
 * five are unused actions: retain P1's eleven real entries verbatim and put
 * P2's seven action masks in this tagged, checked tail. Old cards seed P2
 * from their existing layout. No card offsets, sizes or checksums change. */
static inline unsigned mmx5_bindings_checksum(const uint8_t *p) {
    unsigned v=0x59;for(unsigned i=0;i<8;++i)v=((v<<1)|(v>>7))^p[i];return v&255;
}
static inline int mmx5_bindings_decode(const uint8_t native[32],uint8_t actions[7]) {
    const uint8_t *p=native+22;
    if(p[7]!=0xC5 || p[8]!=2 || p[9]!=mmx5_bindings_checksum(p))return 0;
    unsigned seen=0;
    for(unsigned i=0;i<7;++i) {
        unsigned key=p[i];
        if(!key || (key&(key-1)) || (seen&key))return 0;
        seen|=key;
    }
    memcpy(actions,p,7);return 1;
}
static inline void mmx5_bindings_encode(uint8_t native[32],const uint8_t actions[7]) {
    uint8_t *p=native+22;memcpy(p,actions,7);p[7]=0xC5;p[8]=2;
    p[9]=(uint8_t)mmx5_bindings_checksum(p);
}
#endif
