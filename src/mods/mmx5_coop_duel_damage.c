#include "mmx5_coop_duel_damage.h"
#include <string.h>

unsigned mmx5_coop_duel_attack_damage(unsigned type) {
    switch(type) {
    case 0x00: case 0x3A:return 2;
    case 0x09: case 0x18: case 0x19: case 0x1E:return 3;
    case 0x12: case 0x13: case 0x14: case 0x1A:return 5;
    case 0x15:return 1;
    case 0x16: case 0x17: case 0x2A:return 6;
    case 0x02: case 0x0B: case 0x1D: case 0x21:return 0;
    default:return type<=0x31?4:0;
    }
}

static uint32_t read32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void write32(uint8_t *p, uint32_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value >> 8);
    p[2]=(uint8_t)(value >> 16); p[3]=(uint8_t)(value >> 24);
}
static int buffers_overlap(const void *a, unsigned a_bytes,
                           const void *b, unsigned b_bytes) {
    uintptr_t x=(uintptr_t)a, y=(uintptr_t)b;
    return x<=y ? y-x<a_bytes : x-y<b_bytes;
}
static int valid_body(const uint8_t *body) {
    return body && body[0] && body[2]<=1 && body[4]==1 &&
           (body[0x5C]&0x7Fu) && !body[0xD5] && read32(body+0x54);
}

int mmx5_coop_duel_damage_ready(
        const uint8_t defender[MMX5_COOP_DUEL_BODY_BYTES]) {
    return valid_body(defender) && !(defender[0x5C]&0x80u) &&
           !defender[0x61] && !defender[0xA4] && !defender[0xCC] &&
           !defender[0x7A];
}

int mmx5_coop_duel_damage_target(
        uint8_t proxy[MMX5_COOP_DUEL_PROXY_BYTES],
        const uint8_t defender[MMX5_COOP_DUEL_BODY_BYTES],
        unsigned attacker_seat, unsigned defender_seat,
        uint32_t native_story_table, uint8_t native_token) {
    uint8_t prepared[MMX5_COOP_DUEL_PROXY_BYTES]={0};
    if(!proxy || !valid_body(defender) || attacker_seat>1 || defender_seat>1 ||
       attacker_seat==defender_seat || (native_story_table&1u) ||
       native_story_table<0x80010000u || native_story_table>0x801FFFFEu ||
       buffers_overlap(proxy,MMX5_COOP_DUEL_PROXY_BYTES,
                       defender,MMX5_COOP_DUEL_BODY_BYTES))return 0;
    prepared[0]=1;
    prepared[4]=1;
    memcpy(prepared+8,defender+8,8);
    prepared[0x15]=defender[0x15];
    write32(prepared+0x54,read32(defender+0x54));
    write32(prepared+0x58,native_story_table);
    prepared[0x5C]=defender[0x5C]&0x7Fu;
    prepared[0x61]=defender[0x61] ? defender[0x61] :
                   (uint8_t)!mmx5_coop_duel_damage_ready(defender);
    prepared[0x65]=native_token;
    memcpy(proxy,prepared,sizeof prepared);
    return 1;
}

int mmx5_coop_duel_damage_incoming(
        uint8_t incoming[MMX5_COOP_DUEL_PROXY_BYTES],
        const uint8_t shot[MMX5_COOP_DUEL_PROXY_BYTES],
        unsigned native_amount, int attacker_camera_x, int attacker_camera_y) {
    uint8_t prepared[MMX5_COOP_DUEL_PROXY_BYTES]={0};
    uint32_t x,y;
    if(!incoming || !shot || !shot[0] || (shot[1]&0x80u) ||
       !read32(shot+0x50) || !native_amount || native_amount>127 ||
       buffers_overlap(incoming,MMX5_COOP_DUEL_PROXY_BYTES,
                       shot,MMX5_COOP_DUEL_PROXY_BYTES))return 0;
    x=read32(shot+8);y=read32(shot+12);
    if(shot[0x14]&0x80u) {
        int sx,sy;
        if(attacker_camera_x<-32768 || attacker_camera_x>32767 ||
           attacker_camera_y<-32768 || attacker_camera_y>32767)return 0;
        sx=(int)(x >> 16);sy=(int)(y >> 16);
        if(sx>=32768)sx-=65536;
        if(sy>=32768)sy-=65536;
        /* 2E408 adds the signed native camera origin before overlap.
         * Reject integer overflow into a different16-bit world position. */
        sx+=attacker_camera_x;sy+=attacker_camera_y;
        if(sx<-32768 || sx>32767 || sy<-32768 || sy>32767)return 0;
        x+=(uint32_t)attacker_camera_x << 16;
        y+=(uint32_t)attacker_camera_y << 16;
    }
    prepared[0]=shot[0];
    prepared[1]=shot[1];
    write32(prepared+8,x);write32(prepared+12,y);
    prepared[0x15]=shot[0x15];
    write32(prepared+0x50,read32(shot+0x50));
    prepared[0x60]=(uint8_t)native_amount;
    /* Native317B8 reads+62 as player reaction/capture kind. Story table+63
     * is a different NPC contract; do not copy it into the real hero. */
    prepared[0x62]=0;
    memcpy(incoming,prepared,sizeof prepared);
    return 1;
}

int mmx5_coop_duel_damage_result(
        const uint8_t before[MMX5_COOP_DUEL_BODY_BYTES],
        const uint8_t after[MMX5_COOP_DUEL_BODY_BYTES],
        uint32_t native_injury_result, unsigned *remaining_hp) {
    unsigned remaining;
    if(!before || !after || !remaining_hp || native_injury_result!=1 ||
       !mmx5_coop_duel_damage_ready(before) || before[2]!=after[2] ||
       !(after[0x5C]&0x80u))return 0;
    remaining=after[0x5C]&0x7Fu;
    if(remaining>=before[0x5C])return 0;
    *remaining_hp=remaining;
    return 1;
}
