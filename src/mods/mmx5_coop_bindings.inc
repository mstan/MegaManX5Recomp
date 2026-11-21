/* Native button assignments remain guest data, including across native card
 * saves, current-build snapshots and rollback. Project only around their
 * original consumers; native editing, swaps and X/Zero action rules run. */
static unsigned bindings_seat,bindings_call,bindings_name_call;
static uint32_t bindings_name_frame;
static uint16_t bindings_previous[2];
#define BINDINGS 0x8006FAE4u
static void bindings_capture(uint8_t native[32],uint8_t p2[7]) {
    capture(BINDINGS,native,32);
    if(!mmx5_bindings_decode(native,p2))
        for(unsigned i=0;i<7;++i)p2[i]=native[8+i*2];
}
static void bindings_project(const uint8_t native[32],const uint8_t p2[7],unsigned seat) {
    uint8_t clean[32];memcpy(clean,native,32);memset(clean+22,0,10);
    if(seat)for(unsigned i=0;i<7;++i) {clean[8+i*2]=p2[i];clean[9+i*2]=0;}
    project(BINDINGS,clean,32);
}
static int bindings_mapper(CPUState *cpu,uint32_t address) {
    if(bindings_call || failed)return 0;
    uint8_t native[32],p2[7];bindings_capture(native,p2);
    bindings_project(native,p2,projected!=0);
    bindings_call=1;uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);bindings_call=0;
    mmx5_bindings_encode(native,p2);project(BINDINGS,native,32);
    cpu->gpr[2]=result;return 1;
}
static int bindings_editor(CPUState *cpu,uint32_t address) {
    if(bindings_call || failed || psx_mod_local_view_scope())return 0;
    uint16_t held[2]={input(0),input(1)};
    uint16_t edge[2]={(uint16_t)(held[0]&~bindings_previous[0]),(uint16_t)(held[1]&~bindings_previous[1])};
    memcpy(bindings_previous,held,sizeof held);
    /* Select is outside the eight assignable face/shoulder buttons. */
    if(!psx_mod_read_byte(0x8009A424u) && ((edge[0]|edge[1])&0x100u))bindings_seat^=1;
    uint8_t native[32],p2[7],pad[6];bindings_capture(native,p2);capture(PAD,pad,sizeof pad);
    bindings_project(native,p2,bindings_seat);
    if(bindings_seat) {
        psx_mod_write_half(PAD,(uint16_t)(held[0]|held[1]));
        psx_mod_write_half(PAD+4,(uint16_t)(psx_mod_read_half(PAD+4)|edge[1]));
    }
    bindings_call=1;uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);bindings_call=0;
    for(unsigned i=0;i<7;++i) {
        uint8_t value=psx_mod_read_byte(BINDINGS+8+i*2);
        if(bindings_seat)p2[i]=value;else native[8+i*2]=value;
    }
    mmx5_bindings_encode(native,p2);project(BINDINGS,native,32);project(PAD,pad,sizeof pad);
    cpu->gpr[2]=result;return 1;
}
static void bindings_icon(CPUState *cpu,uint32_t address) {
    (void)address;
    if(failed || !bindings_seat)return;
    uint32_t source=cpu->gpr[3];
    if(source<BINDINGS+8 || source>BINDINGS+20 || (source&1))return;
    uint8_t native[32],p2[7];bindings_capture(native,p2);
    cpu->gpr[5]=p2[(source-BINDINGS-8)/2];
}
static int bindings_name_draw(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(bindings_name_call || failed || psx_mod_read_byte(0x800D1F28u)!=10 ||
       actor<0x800994A0u || actor>=0x8009A0A0u || (actor-0x800994A0u)%0x60u ||
       psx_mod_read_byte(actor+1)!=0 || psx_mod_read_byte(actor+2)!=0x20)return 0;
    uint32_t original=psx_mod_read_word(actor+0x3C);uint8_t frame=psx_mod_read_byte(actor+0x47);
    unsigned count=psx_mod_read_half(original+frame*4u);
    if(count>48 || !bindings_name_frame)return 0;
    uint32_t source=original+psx_mod_read_half(original+frame*4u+2u)*4u;
    for(unsigned i=0;i<count*4;++i)
        psx_mod_write_byte(bindings_name_frame+4+i,psx_mod_read_byte(source+i));
    const char *name=bindings_seat?"PLAYER TWO":"PLAYER ONE";
    for(unsigned i=0;name[i];++i)if(name[i]!=' ') {
        unsigned letter=(unsigned)(name[i]-'A');uint16_t tile=(uint16_t)(7u+(letter/8u)*16u+letter%8u);
        uint32_t at=bindings_name_frame+4u+count++*4u;
        psx_mod_write_byte(at,(uint8_t)(tile>>8));psx_mod_write_byte(at+1,(uint8_t)tile);
        psx_mod_write_byte(at+2,(uint8_t)(-40+(int)i*8));psx_mod_write_byte(at+3,(uint8_t)-24);
    }
    psx_mod_write_half(bindings_name_frame,(uint16_t)count);psx_mod_write_half(bindings_name_frame+2,1);
    psx_mod_write_word(actor+0x3C,bindings_name_frame);psx_mod_write_byte(actor+0x47,0);
    bindings_name_call=1;uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    psx_mod_write_word(actor+0x3C,original);psx_mod_write_byte(actor+0x47,frame);
    bindings_name_call=0;psx_mod_counter_add("mmx5.coop.button-player-label",1);cpu->gpr[2]=result;return 1;
}
