/* Later X4 simultaneous-menu contract, adapted to original X5 MENU.
 * Native body/inventory work runs once per seat; shared fades/world/modal UI
 * run once. Private view callbacks only emit the selected native menu. */
#define PERSONAL_MENU 0x800D4F40u
#define PERSONAL_WEAPON 0x800E9528u
#define PERSONAL_FADE 0x8009A424u
#define PERSONAL_START 0x0800u
#define PERSONAL_CONFIRM 0x0040u
static void personal_menu_read(PersonalMenu *m) {
    capture(PERSONAL_MENU,m->native,sizeof m->native);
    m->original_weapon=psx_mod_read_byte(PERSONAL_WEAPON);
}
static void personal_menu_write(const PersonalMenu *m) {
    project(PERSONAL_MENU,m->native,sizeof m->native);
    psx_mod_write_byte(PERSONAL_WEAPON,m->original_weapon);
}
static void personal_menu_reset(void) {
    memset(personal_menus,0,sizeof personal_menus);
    memset(personal_menu_previous,0,sizeof personal_menu_previous);
    personal_active=personal_initialized=personal_eligible=0;
    personal_menu_call=personal_menu_seat=personal_rendering=personal_shared_call=0;
}
static int personal_menu_busy(unsigned seat) {
    const uint8_t *m=personal_menus[seat].native;
    return m[4]==1 && (m[5]==3 || m[5]==4);
}
static int personal_menu_modal(void) {
    const uint8_t *m=personal_menus[0].native;
    return m[4]==1 && m[5]>=5;
}
static int personal_menu_tank(unsigned seat,uint16_t edge) {
    const uint8_t *m=personal_menus[seat].native;
    if(duel_scope() && m[0x14]>=10 && m[0x14]<=11)return 0;
    return (personal_eligible&(1u<<seat)) && m[4]==1 && m[5]==1 &&
        m[0x14]>=10 && m[0x14]<=12 && (edge&~PERSONAL_START)==PERSONAL_CONFIRM;
}
static int personal_menu_world_return(void) {
    const uint8_t *m=personal_menus[0].native;
    return personal_active && personal_initialized && m[4]==2 && m[5]>=3;
}
static int personal_menu_present(void) {
    return personal_active && personal_initialized && !personal_menu_modal() &&
        !personal_menu_world_return() && psx_mod_read_byte(PLAY)==10 &&
        psx_mod_read_byte(PLAY+1)==2;
}
static void personal_menu_begin(void) {
    memset(personal_menus,0,sizeof personal_menus);
    personal_menu_read(&personal_menus[0]);
    personal_active=1;personal_initialized=0;
    personal_eligible=(unsigned)mmx5_coop_living(&life,0)|
        ((unsigned)mmx5_coop_living(&life,1)<<1);
    for(unsigned seat=0;seat<2;++seat)personal_menu_previous[seat]=input(seat);
}
/* Only P1's step may change the shared fade/music/world. P2's native body
 * weapon installation still runs, including its own deferred art cleanup. */
static int personal_menu_shared(CPUState *cpu,uint32_t address) {
    if(!personal_menu_call || personal_shared_call)return 0;
    if(personal_menu_seat) {cpu->gpr[2]=0;return 1;}
    if(address!=0x80022D98u || (personal_eligible&1u) ||
       !(personal_eligible&2u))return 0;
    personal_shared_call=1;
    enter_second();
    psx_mod_write_byte(PLAY+0x44,saved_play[0x44]);
    psx_mod_write_word(SCRATCH+0x28,resources[3]);
    world_owner=1;
    world_tick(cpu,address);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    world_owner=0;leave_second_context(1);
    personal_shared_call=0;cpu->gpr[2]=result;return 1;
}
static int personal_menu_native_draw(CPUState *cpu,uint32_t address) {
    (void)address;
    int modal=!personal_menu_seat && psx_mod_read_byte(PERSONAL_MENU+4)==1 &&
        psx_mod_read_byte(PERSONAL_MENU+5)>=5;
    if(personal_menu_call && !personal_rendering && (personal_menu_seat || !modal)) {cpu->gpr[2]=0;return 1;}
    return 0;
}
static int personal_menu_world(CPUState *cpu,uint32_t address) {
    /* One callback owns this native boundary. Suppressed P2 menu work must
     * neither update the world nor acknowledge development fixtures. */
    if(personal_shared_call)return 0;
    if(personal_menu_shared(cpu,address))return 1;
    world_tick(cpu,address);return 0;
}
static int personal_menu_dead_commit(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!personal_menu_call || (personal_eligible&(1u<<personal_menu_seat)))return 0;
    /* Do not install equipment onto a dead/withdrawn body's animation state. */
    if(!personal_menu_seat) {
        guest(cpu,0x80022D98u,0,0);guest(cpu,0x80013288u,8,0);
    }
    psx_mod_write_byte(PERSONAL_MENU+5,3);cpu->gpr[2]=0;return 1;
}
static uint32_t personal_menu_step(CPUState *cpu,uint32_t address,unsigned seat,
                                   uint16_t buttons,uint16_t old) {
    PersonalMenu before=personal_menus[seat];
    personal_menu_write(&before);
    uint16_t edge=(uint16_t)(buttons&~old);
    if(seat) {
        buttons&=(uint16_t)~0x0100u;old&=(uint16_t)~0x0100u;edge&=(uint16_t)~0x0100u;
        /* Escape13, Options14 and Parts16 are campaign-owned. Exit15
         * is translated into the shared resume request by the manager. */
        if(before.native[0x14]>=13)edge&=(uint16_t)~PERSONAL_CONFIRM;
    }
    psx_mod_write_half(PAD,buttons);psx_mod_write_half(PAD+2,old);
    psx_mod_write_half(PAD+4,edge);
    if(seat)enter_second();
    personal_menu_call=1;personal_menu_seat=seat;
    uint32_t result=guest(cpu,address,address==0x80034A04u?0:PERSONAL_MENU,0);
    personal_menu_call=0;
    /* Native weapon confirmation requests an immediate single-player exit.
     * It has only changed BODY+93 and played its acknowledgement at this
     * point (336D4/33798); retain that selection until team resume. */
    if(before.native[4]==1 && before.native[5]<3 && before.native[0x14]<10 &&
       psx_mod_read_byte(PERSONAL_MENU+4)==2) {
        psx_mod_write_byte(PERSONAL_MENU+4,1);
        psx_mod_write_byte(PERSONAL_MENU+5,before.native[5]);
        psx_mod_write_byte(PERSONAL_MENU+6,0);
    }
    personal_menu_read(&personal_menus[seat]);
    if(seat) {
        /* Acquisition and W stay shared. With private E, leave_second captures
         * its owner. With shared E, carry the original budget/compaction back
         * across the PLAY restore rather than silently refunding P2's use. */
        uint8_t e[2]={psx_mod_read_byte(PLAY+0x76),psx_mod_read_byte(PLAY+0x77)};
        uint16_t acquired=psx_mod_read_half(PLAY+0x7E);
        uint8_t w=psx_mod_read_byte(PLAY+0x78),dirty=psx_mod_read_byte(0x800C9310u)&1u;
        leave_second();psx_mod_write_byte(PLAY+0x78,w);
        psx_mod_write_half(PLAY+0x7E,acquired);
        if(!separate_etanks) {
            psx_mod_write_byte(PLAY+0x76,e[0]);psx_mod_write_byte(PLAY+0x77,e[1]);
        }
        psx_mod_write_byte(0x800C9310u,(uint8_t)(psx_mod_read_byte(0x800C9310u)|dirty));
    }
    return result;
}
static int personal_menu_tick(CPUState *cpu,uint32_t address) {
    if(!personal_active || personal_menu_call ||
       projected || !enrolled || failed)return 0;
    uint8_t pad[6];capture(PAD,pad,sizeof pad);
    uint16_t buttons[2]={input(0),input(1)},old[2],edge[2];
    for(unsigned seat=0;seat<2;++seat) {
        old[seat]=personal_menu_previous[seat];
        edge[seat]=(uint16_t)(buttons[seat]&~old[seat]);
        personal_menu_previous[seat]=buttons[seat];
    }
    uint32_t result=0;
    if(!personal_initialized) {
        result=personal_menu_step(cpu,address,0,0,0);
        if(personal_menus[0].native[4]==0 && personal_menus[0].native[5]==2) {
            memset(&personal_menus[1],0,sizeof personal_menus[1]);
            personal_menus[1].native[5]=1;
            uint8_t fade=psx_mod_read_byte(PERSONAL_FADE);
            psx_mod_write_byte(PERSONAL_FADE,0);
            personal_menu_step(cpu,0x8003322Cu,1,0,0);
            psx_mod_write_byte(PERSONAL_FADE,fade);personal_initialized=1;
        }
    } else {
        int busy=personal_menu_busy(0)||personal_menu_busy(1);
        int modal=personal_menu_modal();
        int tank[2]={personal_menu_tank(0,edge[0]),personal_menu_tank(1,edge[1])};
        int interactive=personal_menus[0].native[4]==1 && personal_menus[1].native[4]==1;
        /* Native Quit is one shared confirmation, operated by whichever
         * living hero requested it. Keep its fade/world work in P1's native
         * MENU context; P2's inventory and texture state remain private. */
        if(interactive && !busy && !modal && !tank[0] && !tank[1]) {
            for(unsigned seat=0;seat<2;++seat)if((personal_eligible&(1u<<seat)) &&
                    (edge[seat]&0x0100u)) {
                pause_owner=(int)seat;
                buttons[0]|=0x0100u;old[0]&=(uint16_t)~0x0100u;
                break;
            }
        }
        if(modal) {
            unsigned owner=pause_owner==1?1u:0u;
            buttons[0]=buttons[owner];old[0]=old[owner];
        }
        unsigned resume=0;
        for(unsigned seat=0;seat<2;++seat) {
            const uint8_t *m=personal_menus[seat].native;
            resume|=(personal_eligible&(1u<<seat)) &&
                ((edge[seat]&PERSONAL_START) || (m[5]==2 && m[0x14]==15 &&
                  (edge[seat]&PERSONAL_CONFIRM)));
        }
        /* Healing wins over simultaneous resume; blocked edges are consumed. */
        if(interactive && !busy && !modal && !tank[0] && !tank[1] && resume)
            for(unsigned seat=0;seat<2;++seat) {
                personal_menus[seat].native[4]=2;personal_menus[seat].native[5]=0;
            }
        /* Restore the saved gameplay texture regions while the native exit
         * is black, before34E90 installs equipment and draws the world. */
        if(personal_menus[0].native[4]==2 && personal_menus[0].native[5]==2 && menu_active)
            menu_vram(cpu,0);
        unsigned first=personal_menu_busy(1)||(tank[1]&&!tank[0])?1:0;
        for(unsigned turn=0;turn<2;++turn) {
            unsigned seat=first^turn;
            if(modal && seat && !personal_menu_busy(seat))continue;
            int allowed=interactive && (!modal || !seat) &&
                (modal?personal_eligible&(1u<<(pause_owner==1?1:0)):
                       personal_eligible&(1u<<seat));
            uint16_t mask=modal&&!seat?0:PERSONAL_START;
            /* Keep weapon selection and Start; health tanks cannot be spent
             * during PvP, including simultaneous personal-menu input. */
            if(duel_scope() && personal_menus[seat].native[5]==1 &&
               personal_menus[seat].native[0x14]>=10 &&
               personal_menus[seat].native[0x14]<=11)mask|=PERSONAL_CONFIRM;
            if(busy && (tank[seat] || (!seat &&
               (personal_menus[seat].native[5]==6 ||
                (personal_menus[seat].native[5]==2 &&
                 (personal_menus[seat].native[0x14]==13 || personal_menus[seat].native[0x14]==15))))))
                mask|=PERSONAL_CONFIRM;
            result=personal_menu_step(cpu,address,seat,allowed?(uint16_t)(buttons[seat]&~mask):0,
                                     allowed?(uint16_t)(old[seat]&~mask):0);
            busy|=personal_menu_busy(seat);modal|=personal_menu_modal();
            if(psx_mod_read_byte(PLAY)!=10)break;
        }
    }
    personal_menu_write(&personal_menus[0]);project(PAD,pad,sizeof pad);
    if(psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+1)!=2)personal_active=0;
    cpu->gpr[2]=result;return 1;
}
/* Immutable UI uploads must enter the renderer's VRAM journal before CPU
 * VRAM changes. Raw GP0(A0) is not reversible at that boundary. */
static int personal_menu_assets(unsigned seat) {
    unsigned character=seat?counterpart:(counterpart^1u);
    uint16_t pixels[704*80];
    static const unsigned rects[2][4]={{320,176,704,80},{832,256,64,256}};
    for(unsigned i=0;i<2;++i) {
        unsigned x=rects[i][0],y=rects[i][1],w=rects[i][2],h=rects[i][3];
        for(unsigned row=0;row<h;++row)
            memcpy(pixels+row*w,assets[character].ui_pixels+(y+row)*1024+x,w*2);
        gr_vram_transfer_in((int)x,(int)y,(int)w,(int)h,pixels);
    }
    for(unsigned i=0;i<512;++i)pixels[i]=psx_mod_read_half(assets[character].menu_colors+i*2);
    /* Original183B4 rect708D8 is(0,482,256,2), unlike X4's y484. */
    gr_vram_transfer_in(0,482,256,2,pixels);return 1;
}
static int personal_menu_draw(CPUState *cpu,unsigned seat) {
    if(!personal_menu_present() || seat>1 || projected)return 0;
    personal_menu_write(&personal_menus[seat]);personal_menu_assets(seat);
    if(seat)enter_second();
    personal_rendering=1;guest(cpu,0x80018400u,0,0);personal_rendering=0;
    if(seat)leave_second();return 1;
}

static unsigned personal_menu_shade(void) {
    if(!personal_active || !personal_initialized)return 0;
    const uint8_t *m=personal_menus[0].native;
    if(!psx_mod_read_byte(PERSONAL_FADE))return m[4]==2 && m[5]<=2?255:0;
    /* Original1303C emitted this exact subtractive TILE before updating its
     * next shade. Replay the displayed byte, never step the shared task. */
    return psx_mod_read_byte(0x800C9568u+(psx_mod_read_word(SCRATCH)&1u)*16u+4);
}
static void personal_menu_draw_shade(unsigned shade) {
    if(!shade)return;
    uint32_t mode=psx_mod_read_word(0x800D51E8u+(psx_mod_read_word(SCRATCH)&1u)*8u+4);
    if((mode>>24)!=0xE1u)mode=0xE1000045u;
    gpu_set_gp0_source(0);gpu_write_gp0(mode);
    gpu_write_gp0(0x62000000u|shade|(shade<<8)|(shade<<16));
    gpu_write_gp0(0);gpu_write_gp0(0x00F00140u);
}
