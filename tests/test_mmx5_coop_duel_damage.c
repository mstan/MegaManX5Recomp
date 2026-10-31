#include "mmx5_coop_duel_damage.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdint.h>
#include <string.h>

static void put32(uint8_t *p,uint32_t v) {
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|
           (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void body(uint8_t *p) {
    memset(p,0,MMX5_COOP_DUEL_BODY_BYTES);
    p[0]=1;p[2]=1;p[4]=1;p[0x5C]=42;
    put32(p+8,0x01A08000u);put32(p+12,0x01704000u);
    put32(p+0x54,0x800748D0u);
}

int main(void) {
    assert(mmx5_coop_duel_attack_damage(0)==2);
    assert(mmx5_coop_duel_attack_damage(9)==3);
    assert(mmx5_coop_duel_attack_damage(0x12)==5);
    assert(mmx5_coop_duel_attack_damage(0x18)==3);
    assert(mmx5_coop_duel_attack_damage(0x1A)==5);
    assert(mmx5_coop_duel_attack_damage(0x2A)==6);
    assert(!mmx5_coop_duel_attack_damage(2));
    assert(!mmx5_coop_duel_attack_damage(0x21));
    assert(!mmx5_coop_duel_attack_damage(0x80));
    for(unsigned type=0;type<256;++type)assert(mmx5_coop_duel_attack_damage(type)<=6);
    uint8_t defender[MMX5_COOP_DUEL_BODY_BYTES],before[MMX5_COOP_DUEL_BODY_BYTES];
    uint8_t after[MMX5_COOP_DUEL_BODY_BYTES];
    uint8_t proxy[MMX5_COOP_DUEL_PROXY_BYTES],shot[MMX5_COOP_DUEL_PROXY_BYTES];
    uint8_t incoming[MMX5_COOP_DUEL_PROXY_BYTES],shot_copy[MMX5_COOP_DUEL_PROXY_BYTES];
    unsigned hp=999;
    body(defender);memcpy(before,defender,sizeof before);
    assert(mmx5_coop_duel_damage_ready(defender));
    assert(mmx5_coop_duel_damage_target(proxy,defender,0,1,0x80076560u,0x59));
    assert(get32(proxy+8)==get32(defender+8));
    assert(get32(proxy+0x54)==0x800748D0u && get32(proxy+0x58)==0x80076560u);
    assert(proxy[0x5C]==42 && proxy[0x65]==0x59 && !proxy[0x61]);
    assert(!proxy[3] && !get32(proxy+0x50)); /* target cannot touch/draw */
    assert(!memcmp(defender,before,sizeof before));
    assert(!mmx5_coop_duel_damage_target(proxy,defender,1,1,0x80076560u,0));
    assert(!mmx5_coop_duel_damage_target(defender,defender,0,1,0x80076560u,0));
    assert(!memcmp(defender,before,sizeof before));
    assert(!mmx5_coop_duel_damage_target(proxy,defender,0,1,0x80076561u,0));

    /* Invulnerability still needs a native empty scan to clear held token. */
    defender[0x61]=27;
    assert(!mmx5_coop_duel_damage_ready(defender));
    assert(mmx5_coop_duel_damage_target(proxy,defender,0,1,0x80076560u,0x59));
    assert(proxy[0x61]==27 && proxy[0x65]==0x59);
    defender[0x61]=0;defender[0x5C]=0xAA;
    assert(mmx5_coop_duel_damage_target(proxy,defender,0,1,0x80076560u,0x59));
    assert(proxy[0x5C]==42 && proxy[0x61]);
    body(defender);defender[0x7A]=1;
    assert(mmx5_coop_duel_damage_target(proxy,defender,0,1,0x80076560u,0));
    assert(proxy[0x61]);
    defender[0x7A]=2;
    assert(!mmx5_coop_duel_damage_ready(defender));
    assert(mmx5_coop_duel_damage_target(proxy,defender,0,1,0x80076560u,0));
    assert(proxy[0x61]); /* native317B8 would reject an NPC-slot attacker */
    body(defender);defender[0xD5]=0xFF;
    assert(!mmx5_coop_duel_damage_target(proxy,defender,0,1,0x80076560u,0));
    body(defender);defender[0x5C]=0x80;
    assert(!mmx5_coop_duel_damage_target(proxy,defender,0,1,0x80076560u,0));

    memset(shot,0,sizeof shot);shot[0]=0x41;shot[1]=0x18;shot[0x15]=0x40;
    put32(shot+8,0x000C8000u);put32(shot+12,0x00144000u);
    put32(shot+0x50,0x800774C0u);shot[0x14]=0xFF;
    memcpy(shot_copy,shot,sizeof shot);
    assert(mmx5_coop_duel_damage_incoming(incoming,shot,5,100,200));
    assert(get32(incoming+8)==0x00708000u && get32(incoming+12)==0x00DC4000u);
    assert(get32(incoming+0x50)==0x800774C0u && incoming[0x15]==0x40);
    assert(incoming[0x60]==5 && !incoming[0x62] && !incoming[0x14]);
    assert(!memcmp(shot,shot_copy,sizeof shot));
    assert(mmx5_coop_duel_damage_incoming(incoming,shot,5,-32,-40));
    assert(get32(incoming+8)==0xFFEC8000u && get32(incoming+12)==0xFFEC4000u);
    assert(!mmx5_coop_duel_damage_incoming(incoming,shot,0,0,0));
    assert(!mmx5_coop_duel_damage_incoming(incoming,shot,128,0,0));
    assert(!mmx5_coop_duel_damage_incoming(shot,shot,5,0,0));
    assert(!memcmp(shot,shot_copy,sizeof shot));
    assert(!mmx5_coop_duel_damage_incoming(incoming,shot,5,32767,0));

    /* Native player injury, including half-damage, is the authority. */
    body(before);memcpy(after,before,sizeof after);after[0x5C]=0xA8;
    assert(mmx5_coop_duel_damage_result(before,after,1,&hp) && hp==40);
    after[0x5C]=0x80;
    assert(mmx5_coop_duel_damage_result(before,after,1,&hp) && !hp);
    after[0x5C]=0xAA;
    assert(!mmx5_coop_duel_damage_result(before,after,1,&hp));
    after[0x5C]=40;
    assert(!mmx5_coop_duel_damage_result(before,after,1,&hp));
    after[0x5C]=0xA8;
    assert(!mmx5_coop_duel_damage_result(before,after,0,&hp));
    after[2]=0;
    assert(!mmx5_coop_duel_damage_result(before,after,1,&hp));
    return 0;
}
