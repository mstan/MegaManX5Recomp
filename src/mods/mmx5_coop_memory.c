#include "mmx5_coop_memory.h"
#include "mod_plugins.h"
#include "pgxp.h"

void mmx5_coop_capture(uint32_t address,void *data,size_t bytes) {
    uint8_t *p=data;
    while(bytes && (address&3u)) {
        *p++=psx_mod_read_byte(address++);--bytes;
    }
    while(bytes>=4) {
        uint32_t word=psx_mod_read_word(address);
        p[0]=(uint8_t)word;p[1]=(uint8_t)(word>>8);
        p[2]=(uint8_t)(word>>16);p[3]=(uint8_t)(word>>24);
        p+=4;address+=4;bytes-=4;
    }
    while(bytes) {*p++=psx_mod_read_byte(address++);--bytes;}
}
void mmx5_coop_project(uint32_t address,const void *data,size_t bytes) {
    const uint8_t *p=data;
    /* Only these canonical X5 context buffers are plain data memory.
     * Keep device/mod-arena and private replay stores on their original path.
     * Equal host stores also invalidate PGXP shadows, so retain every store
     * whenever precision is armed, including a temporary suppression scope. */
    static const uint32_t spans[][2]={
        {0x800942A8u,0x80094398u}, /* Charge glow. */
        {0x80098120u,0x800994A0u}, /* Private shots. */
        {0x8009A0A0u,0x8009A418u}, /* Body, cameras and dash trails. */
        {0x800D1C00u,0x800D1F20u}, /* PLAY and both native ride bodies. */
        {0x800E9308u,0x800E9328u}, /* Temporary terrain probe. */
        {0x1F800000u,0x1F800400u}  /* Scratchpad, excluding device registers. */
    };
    int pure=0;
    for(size_t i=0;i<sizeof spans/sizeof spans[0];++i)
        if(address>=spans[i][0] && address<spans[i][1] && bytes<=spans[i][1]-address) {
            pure=1;break;
        }
    int compare=pure && !pgxp_enabled() &&
        !psx_mod_local_view_scope();
    while(bytes && (address&3u)) {
        if(!compare || psx_mod_read_byte(address)!=*p)psx_mod_write_byte(address,*p);
        ++address;++p;--bytes;
    }
    while(bytes>=4) {
        uint32_t word=(uint32_t)p[0]|(uint32_t)p[1]<<8|
            (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
        if(!compare || psx_mod_read_word(address)!=word)psx_mod_write_word(address,word);
        p+=4;address+=4;bytes-=4;
    }
    while(bytes) {
        if(!compare || psx_mod_read_byte(address)!=*p)psx_mod_write_byte(address,*p);
        ++address;++p;--bytes;
    }
}
