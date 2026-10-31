#include "mmx5_coop_memory.h"
#undef NDEBUG
#include <assert.h>
#include <string.h>
static uint8_t ram[2048];
static uint8_t scratch[1028],mmio[16],edge[8],arena[16];
static unsigned reads,writes;
static int precision,local_view;
int pgxp_enabled(void) {return precision;}
int psx_mod_local_view_scope(void) {return local_view;}
static uint8_t *memory(uint32_t address) {
    if(address<sizeof ram)return ram+address;
    if(address>=0x80098120u && address<0x80098120u+sizeof ram)return ram+(address-0x80098120u);
    if(address>=0x1F800000u && address<0x1F800000u+sizeof scratch)return scratch+(address-0x1F800000u);
    if(address>=0x1F801040u && address<0x1F801040u+sizeof mmio)return mmio+(address-0x1F801040u);
    if(address>=0x8009949Cu && address<0x8009949Cu+sizeof edge)return edge+(address-0x8009949Cu);
    if(address>=0x9F000000u && address<0x9F000000u+sizeof arena)return arena+(address-0x9F000000u);
    assert(!"unmapped fake memory");return NULL;
}
uint8_t psx_mod_read_byte(uint32_t address) {++reads;return *memory(address);}
uint32_t psx_mod_read_word(uint32_t address) {
    ++reads;assert(!(address&3u));uint8_t *p=memory(address);
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|
        (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
void psx_mod_write_byte(uint32_t address,uint8_t value) {++writes;*memory(address)=value;}
void psx_mod_write_word(uint32_t address,uint32_t value) {
    ++writes;assert(!(address&3u));uint8_t *p=memory(address);
    for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(value>>(i*8));
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

    /* The actual private-shot span exercises every alignment and tail.
     * A second identical projection must leave all bytes alone with no stores. */
    for(unsigned address=32;address<36;++address)for(unsigned offset=0;offset<4;++offset)
        for(unsigned length=0;length<=513;++length) {
            for(unsigned i=0;i<sizeof ram;++i)ram[i]=(uint8_t)(i*37+5);
            memcpy(expected,ram,sizeof ram);memcpy(expected+address,input+offset,length);
            mmx5_coop_project(0x80098120u+address,input+offset,length);
            assert(!memcmp(ram,expected,sizeof ram));
            writes=0;
            mmx5_coop_project(0x80098120u+address,input+offset,length);
            assert(!writes && !memcmp(ram,expected,sizeof ram));
        }
    memcpy(captured,ram+128,252);reads=writes=0;
    mmx5_coop_project(0x800981A0u,captured,252);
    assert(reads==63 && !writes);
    captured[5]^=1;reads=writes=0;
    mmx5_coop_project(0x800981A0u,captured,252);
    assert(reads==63 && writes==1 && !memcmp(ram+128,captured,252));

    /* Equal host stores still clear PGXP shadows. An armed engine or a
     * private-view replay must retain the complete original store path. */
    precision=1;reads=writes=0;
    mmx5_coop_project(0x800981A0u,captured,252);
    assert(!reads && writes==63);
    precision=0;local_view=1;reads=writes=0;
    mmx5_coop_project(0x800981A0u,captured,252);
    assert(!reads && writes==63);local_view=0;

    memset(scratch,0x43,sizeof scratch);memset(captured,0x43,31);reads=writes=0;
    mmx5_coop_project(0x1F800001u,captured,31);
    assert(reads && !writes);
    /* Devices, mod arenas and a span crossing a known buffer boundary must
     * receive all stores, without speculative reads that could have effects. */
    memset(input,0,sizeof input);memset(mmio,0,sizeof mmio);reads=writes=0;
    mmx5_coop_project(0x1F801040u,input,sizeof mmio);
    assert(!reads && writes==4);
    memset(arena,0,sizeof arena);reads=writes=0;
    mmx5_coop_project(0x9F000000u,input,sizeof arena);
    assert(!reads && writes==4);
    memset(edge,0,sizeof edge);reads=writes=0;
    mmx5_coop_project(0x8009949Cu,input,sizeof edge);
    assert(!reads && writes==2);
    reads=writes=0;
    mmx5_coop_project(0xFFFFFFFFu,input,0);
    assert(!reads && !writes);
    return 0;
}
