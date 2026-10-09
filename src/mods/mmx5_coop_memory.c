#include "mmx5_coop_memory.h"
#include "mod_plugins.h"

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
    while(bytes && (address&3u)) {
        psx_mod_write_byte(address++,*p++);--bytes;
    }
    while(bytes>=4) {
        uint32_t word=(uint32_t)p[0]|(uint32_t)p[1]<<8|
            (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
        psx_mod_write_word(address,word);
        p+=4;address+=4;bytes-=4;
    }
    while(bytes) {psx_mod_write_byte(address++,*p++);--bytes;}
}
