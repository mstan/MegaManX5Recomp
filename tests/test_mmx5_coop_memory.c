#include "mmx5_coop_memory.h"
#undef NDEBUG
#include <assert.h>
#include <string.h>
static uint8_t ram[2048];
static unsigned reads,writes;
uint8_t psx_mod_read_byte(uint32_t address) {++reads;assert(address<sizeof ram);return ram[address];}
uint32_t psx_mod_read_word(uint32_t address) {
    ++reads;assert(!(address&3u) && address+4<=sizeof ram);
    return (uint32_t)ram[address]|(uint32_t)ram[address+1]<<8|
        (uint32_t)ram[address+2]<<16|(uint32_t)ram[address+3]<<24;
}
void psx_mod_write_byte(uint32_t address,uint8_t value) {++writes;assert(address<sizeof ram);ram[address]=value;}
void psx_mod_write_word(uint32_t address,uint32_t value) {
    ++writes;assert(!(address&3u) && address+4<=sizeof ram);
    for(unsigned i=0;i<4;++i)ram[address+i]=(uint8_t)(value>>(i*8));
}
int main(void) {
    uint8_t expected[sizeof ram],captured[1024],input[1024];
    for(unsigned i=0;i<sizeof input;++i)input[i]=(uint8_t)(i*71+19);
    /* Every guest and host alignment, zero-length and partial-word tails.
     * Check adjacent bytes as well as the complete payload. */
    for(unsigned address=32;address<36;++address)for(unsigned offset=0;offset<4;++offset)
        for(unsigned length=0;length<=513;++length) {
            for(unsigned i=0;i<sizeof ram;++i)ram[i]=(uint8_t)(i*37+5);
            memcpy(expected,ram,sizeof ram);
            memset(captured,0xA5,sizeof captured);
            mmx5_coop_capture(address,captured+offset,length);
            assert(!memcmp(captured+offset,expected+address,length));
            if(offset)assert(captured[offset-1]==0xA5);
            assert(captured[offset+length]==0xA5);
            memcpy(expected+address,input+offset,length);
            mmx5_coop_project(address,input+offset,length);
            assert(!memcmp(ram,expected,sizeof ram));
        }
    /* A complete native camera needs 63 runtime accesses instead of 252. */
    reads=writes=0;
    mmx5_coop_capture(128,captured,252);
    mmx5_coop_project(512,captured,252);
    assert(reads==63 && writes==63);
    return 0;
}
