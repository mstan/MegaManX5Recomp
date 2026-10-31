#include "mmx5_coop_texture_cache.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(v) do { if(!(v)) { fprintf(stderr,"line %d: %s\n",__LINE__,#v);exit(1); } } while(0)
#define WORLD 0x1000u
#define PRIMARY (WORLD+2*0x40000u)
#define HUD (PRIMARY+2*0x40000u)
static uint32_t ram[(HUD+2*0x8000u)/4];
static uint32_t saved[sizeof ram/sizeof ram[0]];
static unsigned reads;
static uint32_t read_word(uint32_t at) {
    CHECK(!(at&3) && at/4<sizeof ram/sizeof ram[0]);++reads;return ram[at/4];
}
static uint32_t buffer_at(unsigned buffer) {
    return buffer<4?(buffer<2?WORLD:PRIMARY)+(buffer&1u)*0x40000u+40000u:
        HUD+(buffer&1u)*0x8000u+0x3000u;
}
static void triangle(uint32_t at,unsigned id) {
    ram[at/4]=0x09FFFFFFu;ram[at/4+1]=0x34808080u;
    ram[at/4+4]=(id&255u)<<24;ram[at/4+7]=(id>>8)<<24;
}
static void pair(uint32_t at,unsigned id) {triangle(at,id);triangle(at+40,id);}
static unsigned slot(unsigned count,unsigned first,unsigned start) {
    return mmx5_coop_texture_cache_slot(count,(uint16_t)first,start,WORLD,PRIMARY,HUD,read_word);
}
int main(void) {
    CHECK(slot(4095,0x6000,0)==4095 && !reads);
    for(unsigned b=0;b<6;++b)pair(buffer_at(b),0x6000+b);
    pair(WORLD+40000+79920,0x6FFF);
    pair(PRIMARY+0x40000+40000+79920,0x6006);
    pair(HUD+0x8000+0x3000+0x4FB0,0x6007);
    CHECK(slot(4096,0x6000,4095)==8);
    CHECK(slot(4096,0xA000,0)==0);
    memcpy(saved,ram,sizeof ram);
    unsigned restored=slot(4096,0x6000,4095);
    memset(ram,0,sizeof ram);CHECK(slot(4096,0x6000,4095)==4095);
    memcpy(ram,saved,sizeof ram);CHECK(slot(4096,0x6000,4095)==restored);

    /* Wrong tag/opcode or IDs in another namespace do not pin a slot. */
    memset(ram,0,sizeof ram);
    triangle(buffer_at(0),0x6000);ram[buffer_at(0)/4]=0x08FFFFFFu;
    triangle(buffer_at(1),0x6000);ram[buffer_at(1)/4+1]=0x30808080u;
    triangle(buffer_at(2),0x7000);CHECK(slot(4096,0x6000,0)==0);

    /* More than a pool's lifetime capacity, with live IDs in all six buffers.
     * A replacement must preserve every current/previous world/HUD packet. */
    const unsigned bases[]={0x6000,0x7000,0xA000,0xB000};
    for(unsigned pool=0;pool<4;++pool) {
        memset(ram,0,sizeof ram);
        unsigned live[6][64];memset(live,0xFF,sizeof live);
        for(unsigned i=0;i<5000;++i) {
            unsigned buffer=(i/64)%6,index=i%64;
            unsigned chosen=slot(4096,bases[pool],i);
            CHECK(chosen<4096);
            for(unsigned b=0;b<6;++b)for(unsigned j=0;j<64;++j)CHECK(chosen!=live[b][j]);
            pair(buffer_at(buffer)+index*80u,bases[pool]+chosen);live[buffer][index]=chosen;
        }
    }
    memset(ram,0,sizeof ram);
    for(unsigned i=0;i<4096;++i)triangle(buffer_at(i/2000)+(i%2000)*40,0x6000+i);
    CHECK(slot(4096,0x6000,0)==UINT_MAX);
    CHECK(slot(4097,0x6000,0)==UINT_MAX);
    CHECK(mmx5_coop_texture_cache_slot(4096,0x6000,0,WORLD,0,HUD,read_word)==UINT_MAX);
    CHECK(mmx5_coop_texture_cache_slot(4096,0x6000,0,WORLD,PRIMARY,HUD,NULL)==UINT_MAX);
    puts("texture cache: sustained reuse pins both X5 world arenas and both HUD buffers");
    return 0;
}
