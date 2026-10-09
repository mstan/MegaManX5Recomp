/* SLUS-01334 native co-op development implementation.
 * Original 80022D98 runs the shared world once. The hook at its attack-pool
 * boundary adds only the counterpart's input, body, attacks and terrain pass.
 */
#include "mod_plugins.h"
#include "mod_netplay.h"
#include "cpu_state.h"
#include "sio.h"
#include "gpu.h"
#include "mmx5_coop_assets.h"
#include "mmx5_coop_sprite_packet.h"
#include "mmx5_coop_lifecycle.h"
#include <stdlib.h>
#include <string.h>

#ifndef PSX_NO_DEBUG_TOOLS
#define ID "mmx5.coop"
#define PLAYER 0x8009A0A0u
#define PLAY 0x800D1C00u
#define PAD 0x800C931Cu
#define SHOTS 0x80098120u
#define TRAILS 0x8009A2F8u
#define CAMERA 0x8009A1F8u
#define SCRATCH 0x1F800000u
#define FRAME_BYTES 0x40000u

typedef struct {
    uint8_t body[0x158], shots[0x1380], trails[0x120];
} PlayerContext;
typedef struct {
    uint8_t *graphics;
    size_t graphics_bytes;
    uint32_t sprites, assembly, colors, palette, menu_colors, menu_layout;
    uint16_t *ui_pixels;
    uint16_t hud_colors[16];
    unsigned loaded;
} CharacterAssets;
typedef struct {
    uint32_t source;
    uint32_t generation;
    uint16_t id, tile;
    uint8_t colors[32];
    uint32_t pixel_hash,pixel_count;
    uint16_t *pixels;
} SpriteBank;
static CharacterAssets assets[2];
static PlayerContext first, second;
static uint8_t saved_play[0x1C0], saved_camera[0xFC];
static uint32_t resources[6];
static const unsigned resource_offsets[6]={0x14,0x1C,0x24,0x28,0x38,0x3C};
static SpriteBank banks[4096];
static unsigned bank_count, counterpart, projected, busy, enrolled, failed, ready, camera_call;
static unsigned stage_call, player_call, world_owner, render_call;
static unsigned camera_update_call;
static uint32_t world_packets;
static Mmx5CoopLifecycle life;
typedef struct {uint32_t x,y,assembly;uint8_t layer,valid;} DeathOrigin;
static DeathOrigin death_origins[128];
static unsigned effect_allocate_call,effect_init_call;
static uint32_t fixture_sequence;
static uint32_t arena, primary_arena, primary_packets, diagnostic, ticks, world_ticks, asset_generation;
static unsigned render_primary;
static uint16_t previous_input;
static uint8_t returning_body[0x158];
static int32_t join_y;
static unsigned join_phase;
typedef struct {uint16_t page,clut,uv,id;uint16_t colors[16];unsigned character;} UiBank;
static UiBank ui_banks[4096];
static unsigned ui_bank_count,hud_call,hud_seat,hud_fade=128;
static uint32_t hud_arena;
typedef struct {unsigned x,y,w,h;uint16_t *pixels;} MenuBackup;
static MenuBackup menu_backups[32];
static unsigned menu_backup_count,menu_active,upload_call;
static int pause_owner=-1,p2_start_previous;
static void menu_vram(CPUState *cpu,int active);
static unsigned ever_enrolled,last_stage,section_pending,stage_departed;
typedef struct {uint32_t actor;uint8_t token[2];} HitHistory;
static HitHistory hit_history[256];
static unsigned hit_count,allocation_call,solid_call,contact_call;
typedef struct {uint8_t sides,carried;} SolidHistory;
static SolidHistory solid_history[64];
typedef struct {uint32_t actor;uint8_t seat;} PickupOwner;
static PickupOwner pickup_owners[128];
static unsigned pickup_call;
static unsigned scene_owner,scene_phase,scene_call,door_call;
static uint8_t scene_body[0x158];
static int32_t scene_y;
static unsigned keep_zero;
static void scene_tick(CPUState *cpu);
static void scene_begin(unsigned seat);
static int scene_passenger(unsigned seat) {return scene_owner && scene_owner!=seat+1;}
static void enforce_roster(void) {
    if(!enrolled || !counterpart || mmx5_coop_zero_available(
        psx_mod_read_byte(PLAY+0x79),psx_mod_read_byte(PLAY+0xCB),keep_zero))return;
    life.status[1]=MMX5_COOP_UNAVAILABLE;second.body[3]=0;second.body[4]=3;
    memset(second.shots,0,sizeof second.shots);memset(second.trails,0,sizeof second.trails);
    if(scene_owner==1)scene_phase=0;
}

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void capture(uint32_t address,void *data,size_t bytes) {
    uint8_t *p=data;
    for(size_t i=0;i<bytes;++i)p[i]=psx_mod_read_byte(address+(uint32_t)i);
}
static void project(uint32_t address,const void *data,size_t bytes) {
    const uint8_t *p=data;
    for(size_t i=0;i<bytes;++i)psx_mod_write_byte(address+(uint32_t)i,p[i]);
}
static void capture_player(PlayerContext *p) {
    capture(PLAYER,p->body,sizeof p->body);
    capture(SHOTS,p->shots,sizeof p->shots);
    capture(TRAILS,p->trails,sizeof p->trails);
}
static void project_player(const PlayerContext *p) {
    project(PLAYER,p->body,sizeof p->body);
    project(SHOTS,p->shots,sizeof p->shots);
    project(TRAILS,p->trails,sizeof p->trails);
}
static uint32_t guest(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1) {
    uint32_t regs[32],data[32],ctrl[32];
    uint32_t pc=cpu->pc,hi=cpu->hi,lo=cpu->lo;
    memcpy(regs,cpu->gpr,sizeof regs);memcpy(data,cpu->gte_data,sizeof data);
    memcpy(ctrl,cpu->gte_ctrl,sizeof ctrl);
    cpu->pc=0;cpu->gpr[4]=a0;cpu->gpr[5]=a1;
    psx_dispatch_call(cpu,address,regs[31]);
    uint32_t result=cpu->gpr[2];
    memcpy(cpu->gpr,regs,sizeof regs);memcpy(cpu->gte_data,data,sizeof data);
    memcpy(cpu->gte_ctrl,ctrl,sizeof ctrl);cpu->pc=pc;cpu->hi=hi;cpu->lo=lo;
    return result;
}
static void arrival_grace(CPUState *cpu) {
    /* Original contact checks A4; 38C54..38C7C clears it when timer61 ends.
     * Setting only the timer leaves a healthy returning actor vulnerable. */
    psx_mod_write_byte(PLAYER+0x61,60);psx_mod_write_byte(PLAYER+0xA4,1);
    guest(cpu,0x8003C5D8,PLAYER,0);
}
static uint8_t *disc_file(const char *name,uint32_t *size) {
    if(!psx_mod_read_disc_file(name,NULL,0,size) || !*size)return NULL;
    uint8_t *data=malloc(*size);
    if(data && psx_mod_read_disc_file(name,data,*size,size))return data;
    free(data);return NULL;
}
static int ui_upload(CharacterAssets *character,const Mmx5AssetView *asset) {
    unsigned type=asset->type&255,layout=(asset->type>>8)&255;
    if(type>=19 || layout>2 || asset->size%2048)return 0;
    unsigned x=psx_mod_read_half(0x80070368u+type*4);
    unsigned y=psx_mod_read_half(0x8007036Au+type*4),base_y=y&256;
    for(size_t at=0;at<asset->size;at+=2048) {
        if(x+64>1024 || y+16>512)return 0;
        for(unsigned row=0;row<16;++row)for(unsigned col=0;col<64;++col) {
            const uint8_t *p=asset->data+at+(row*64+col)*2;
            character->ui_pixels[(y+row)*1024+x+col]=(uint16_t)(p[0]|p[1]<<8);
        }
        if(y==base_y+(layout==1?160:240)) {x+=64;y=layout==2?176:base_y;}
        else y+=16;
    }
    return 1;
}
static int load_assets(void) {
    uint32_t bytes;
    uint8_t *dat=disc_file("ROCK_X5.DAT",&bytes);
    if(!dat)return 0;
    for(unsigned c=0;c<2;++c) {
        Mmx5AssetView graphics,assembly,colors;
        unsigned record=c?90:88;
        if(!mmx5_coop_dat_asset(dat,bytes,record,3,&graphics) || graphics.type!=2 ||
           !mmx5_coop_dat_asset(dat,bytes,record,9,&assembly) || assembly.type!=3 ||
           !mmx5_coop_dat_asset(dat,bytes,record,0,&colors) || colors.type!=9 ||
           graphics.size>0x40000 || assembly.size>0xC000 || colors.size>0x4000) {
            free(dat);return 0;
        }
        assets[c].graphics=malloc(graphics.size);
        if(!assets[c].graphics) {free(dat);return 0;}
        assets[c].graphics_bytes=graphics.size;
        memcpy(assets[c].graphics,graphics.data,graphics.size);
        project(assets[c].sprites,graphics.data,graphics.size);
        project(assets[c].assembly,assembly.data,assembly.size);
        project(assets[c].colors,colors.data,colors.size);
        project(assets[c].palette,colors.data,colors.size);
        Mmx5AssetView menu_colors,menu_layout;
        if(!mmx5_coop_dat_asset(dat,bytes,record,7,&menu_colors) || menu_colors.type!=17 || menu_colors.size>0x400 ||
           !mmx5_coop_dat_asset(dat,bytes,record,8,&menu_layout) || menu_layout.type!=18 || menu_layout.size>0x1800) {
            free(dat);return 0;
        }
        project(assets[c].menu_colors,menu_colors.data,menu_colors.size);
        project(assets[c].menu_layout,menu_layout.data,menu_layout.size);
        /* DAT 1/2 type-5 palettes supply the native X/Zero UI badge CLUT.
         * Original-disc captures identify CLUT 7841 at member offset 220. */
        Mmx5AssetView ui_palette;
        if(!mmx5_coop_dat_asset(dat,bytes,c+1,0,&ui_palette) || ui_palette.type!=5 || ui_palette.size<0x240) {
            free(dat);return 0;
        }
        for(unsigned i=0;i<16;++i)assets[c].hud_colors[i]=(uint16_t)(ui_palette.data[0x220+i*2]|
            ((uint16_t)ui_palette.data[0x221+i*2]<<8));
        assets[c].ui_pixels=calloc(1024*512,sizeof(uint16_t));
        if(!assets[c].ui_pixels) {free(dat);return 0;}
        for(unsigned i=0;i<16;++i) {
            Mmx5AssetView item;
            if(!mmx5_coop_dat_asset(dat,bytes,record,i,&item)) {free(dat);return 0;}
            if((item.type&0x10000) && !ui_upload(&assets[c],&item)) {free(dat);return 0;}
        }
        assets[c].loaded=1;
    }
    free(dat);return 1;
}
static void enter_second(void) {
    projected=1;capture_player(&first);capture(PLAY,saved_play,sizeof saved_play);
    capture(CAMERA,saved_camera,sizeof saved_camera);
    project_player(&second);
    const uint32_t values[6]={assets[counterpart].sprites,assets[counterpart].assembly,
        assets[counterpart].colors,assets[counterpart].palette,assets[counterpart].menu_colors,assets[counterpart].menu_layout};
    for(unsigned i=0;i<6;++i) {
        resources[i]=psx_mod_read_word(SCRATCH+resource_offsets[i]);
        psx_mod_write_word(SCRATCH+resource_offsets[i],values[i]);
    }
    psx_mod_write_byte(PLAY+0x44,(uint8_t)counterpart);
    /* Native 3D5E0 indexes parts/ability flags by the roster entry, separately
     * from PLAYER+2. Entry 5 is Zero; the counterpart X uses native base X. */
    psx_mod_write_byte(PLAY+0x49,(uint8_t)(counterpart?5:0));
}
static void leave_second_context(int shared_world) {
    capture_player(&second);project_player(&first);
    if(shared_world) {
        psx_mod_write_byte(PLAY+0x44,saved_play[0x44]);psx_mod_write_byte(PLAY+0x49,saved_play[0x49]);
    }
    else {
        /* Native rank counters are shared even when a private body pass
         * records damage. Timers and scene flags remain world-owned. */
        uint32_t hits=psx_mod_read_word(PLAY+0xB4),damage=psx_mod_read_word(PLAY+0xB8);
        project(PLAY,saved_play,sizeof saved_play);
        psx_mod_write_word(PLAY+0xB4,hits);psx_mod_write_word(PLAY+0xB8,damage);
        project(CAMERA,saved_camera,sizeof saved_camera);
    }
    for(unsigned i=0;i<6;++i)psx_mod_write_word(SCRATCH+resource_offsets[i],resources[i]);
    projected=0;
}
static void leave_second(void) {leave_second_context(0);}
static void diagnostics(void) {
    if(!diagnostic)return;
    psx_mod_write_word(diagnostic,ticks);psx_mod_write_word(diagnostic+4,ready);
    psx_mod_write_word(diagnostic+8,enrolled);psx_mod_write_word(diagnostic+12,failed);
    psx_mod_write_word(diagnostic+16,counterpart);psx_mod_write_word(diagnostic+20,bank_count);
    psx_mod_write_word(diagnostic+24,world_ticks);
    psx_mod_write_word(diagnostic+28,psx_mod_read_word(PLAY+0x1B4));
    psx_mod_write_byte(diagnostic+0x40,(uint8_t)life.status[0]);
    psx_mod_write_byte(diagnostic+0x41,(uint8_t)life.status[1]);
    psx_mod_write_byte(diagnostic+0x42,life.wipe);
    psx_mod_write_byte(diagnostic+0x43,(uint8_t)world_owner);
    psx_mod_write_word(diagnostic+0x44,life.select_frames);
    psx_mod_write_byte(diagnostic+0x4C,(uint8_t)scene_owner);
    psx_mod_write_byte(diagnostic+0x4D,(uint8_t)scene_phase);
    project(diagnostic+0x100,second.body,sizeof second.body);
    if(!projected)capture(PLAYER,first.body,sizeof first.body);
    project(diagnostic+0x300,first.body,sizeof first.body);
}
static void reset(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;if(busy || projected)return;
    /* Original 3BBC8..3BCE8 uses PLAY+1E == 1 for saved HP/ammo/weapon
     * continuation. A team wipe or campaign departure starts a fresh team. */
    section_pending=ever_enrolled && !stage_departed && !life.wipe &&
        last_stage==psx_mod_read_byte(PLAY+0x0C) && psx_mod_read_byte(PLAY+0x1E)==1;
    enrolled=0;previous_input=0;
    if(!section_pending) {memset(&second,0,sizeof second);mmx5_coop_lifecycle_respawn(&life);}
    else {
        if(life.status[0]==MMX5_COOP_DYING)life.status[0]=MMX5_COOP_FALLEN;
        if(life.status[1]==MMX5_COOP_DYING)life.status[1]=MMX5_COOP_FALLEN;
        if(life.status[1]==MMX5_COOP_LEAVING)life.status[1]=MMX5_COOP_ABSENT;
        if(life.status[1]==MMX5_COOP_JOINING)life.status[1]=MMX5_COOP_ALIVE;
        life.select_frames=life.select_armed=0;
    }
    memset(hit_history,0,sizeof hit_history);hit_count=0;
    memset(solid_history,0,sizeof solid_history);
    memset(pickup_owners,0,sizeof pickup_owners);
    ++asset_generation;
    join_phase=0;
    scene_owner=scene_phase=0;
    pause_owner=-1;p2_start_previous=0;
}
static void observe_stage(void) {
    unsigned mode=psx_mod_read_byte(PLAY);
    if(ever_enrolled && mode!=8 && mode!=10)stage_departed=1;
}
static void world_tick(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
#ifndef PSX_NO_DEBUG_TOOLS
    if(diagnostic && !psx_mod_netplay_is_active()) {
        uint32_t sequence=psx_mod_read_word(diagnostic+0x50);
        if(sequence!=fixture_sequence) {
            unsigned command=psx_mod_read_byte(diagnostic+0x54),seat=psx_mod_read_byte(diagnostic+0x55)&1;
            unsigned resident=world_owner?1:0;
            if(command==1) {
                /* Enter the original fatal-HP path at a native world boundary;
                 * death poses, effects, retry and reload stay native. */
                if(seat==resident)psx_mod_write_byte(PLAYER+0x5C,0x80);
                else (seat?second.body:first.body)[0x5C]=0x80;
            }
            if(command==2) {
                unsigned hp=psx_mod_read_byte(diagnostic+0x56);
                uint16_t ammo=psx_mod_read_half(diagnostic+0x58);
                if(seat==resident) {
                    psx_mod_write_byte(PLAYER+0x5C,(uint8_t)hp);
                    for(unsigned i=0;i<9;++i)psx_mod_write_half(PLAYER+0xA8+i*2,ammo);
                } else {
                    uint8_t *body=seat?second.body:first.body;body[0x5C]=(uint8_t)hp;
                    for(unsigned i=0;i<9;++i) {body[0xA8+i*2]=(uint8_t)ammo;body[0xA9+i*2]=(uint8_t)(ammo>>8);}
                }
            }
            if(command==3) {
                /* Allocate an original item at the requested actor. This is
                 * a controlled ownership scenario, not stage progression. */
                uint32_t actor=guest(cpu,0x8002C938,0,0);
                if(actor) {
                    uint8_t *body=seat?second.body:first.body;
                    uint32_t x=seat==resident?psx_mod_read_word(PLAYER+8):le32(body+8);
                    uint32_t y=seat==resident?psx_mod_read_word(PLAYER+12):le32(body+12);
                    psx_mod_write_byte(actor+2,psx_mod_read_byte(diagnostic+0x56));
                    psx_mod_write_byte(actor+1,0x2F);
                    psx_mod_write_word(actor+8,x);psx_mod_write_word(actor+12,y);
                    psx_mod_write_word(actor+0x10,1);guest(cpu,0x800535C8,actor,0);
                }
                psx_mod_write_word(diagnostic+0x78,actor);
            }
            if(command==4) {
                /* Native shared energy-tank capacity and stored contents. */
                psx_mod_write_half(PLAY+0x7E,(uint16_t)(psx_mod_read_half(PLAY+0x7E)|0x1000));
                psx_mod_write_byte(PLAY+0x76,psx_mod_read_byte(diagnostic+0x56));
            }
            if(command==5 || command==6) {
                /* Original stand/script lock and release, without changing
                 * campaign/room selection or forcing a destination. */
                guest(cpu,command==5?0x8003D29C:0x8003D2C4,0x14,0x40);
            }
            fixture_sequence=sequence;
            psx_mod_write_word(diagnostic+0x60,sequence);
        }
    }
#endif
    if(!busy && (!projected || world_owner))++world_ticks;
    primary_packets=primary_arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_BYTES;
}
static int enroll(CPUState *cpu) {
    if(!psx_mod_texture_banks_supported())return 0;
    unsigned character=psx_mod_read_byte(PLAYER+2);
    if(character>1)return 0;
    counterpart=character^1;
    uint32_t x=psx_mod_read_word(PLAYER+8),y=psx_mod_read_word(PLAYER+12);
    uint8_t inventory[0x158];memcpy(inventory,second.body,sizeof inventory);
    memset(&second,0,sizeof second);
    enter_second();psx_mod_write_byte(PLAY+0x1E,0);
    guest(cpu,0x8003BAC8,0,0);
    if(section_pending) {
        /* Transport the body through native init, retaining its own resources
         * instead of copying the singleton P1 continuation cache. */
        for(unsigned i=0x5C;i<=0x5F;++i)psx_mod_write_byte(PLAYER+i,inventory[i]);
        project(PLAYER+0xA8,inventory+0xA8,32);
        psx_mod_write_byte(PLAYER+0x93,inventory[0x93]);psx_mod_write_byte(PLAYER+0xFB,inventory[0xFB]);
        guest(cpu,0x8003DF20,PLAYER,inventory[0x93]);guest(cpu,0x8003EE38,PLAYER,0);
    }
    psx_mod_write_word(PLAYER+8,x+(24u<<16));psx_mod_write_word(PLAYER+12,y);
    psx_mod_write_word(PLAYER+0x18,x+(24u<<16));psx_mod_write_word(PLAYER+0x1C,y);
    psx_mod_write_byte(PLAYER+3,1);psx_mod_write_byte(PLAYER+4,1);
    /* Original arrival at 3553C/35570 initializes roster parts before the
     * movement constants and idle pose; 3BAC8 alone does not do that. */
    guest(cpu,0x8003D5E0,PLAYER,0);guest(cpu,0x8003B65C,PLAYER,0);guest(cpu,0x8003A670,PLAYER,0);
    guest(cpu,0x8002FB58,PLAYER,0);leave_second();
    if(counterpart && !mmx5_coop_zero_available(psx_mod_read_byte(PLAY+0x79),psx_mod_read_byte(PLAY+0xCB),keep_zero))
        life.status[1]=MMX5_COOP_UNAVAILABLE;
    if(life.status[1]==MMX5_COOP_FALLEN || life.status[1]==MMX5_COOP_ABSENT || life.status[1]==MMX5_COOP_UNAVAILABLE) {
        second.body[3]=0;second.body[4]=3;
        if(life.status[1]==MMX5_COOP_ABSENT)memcpy(returning_body,second.body,sizeof returning_body);
    }
    if(section_pending && life.status[0]==MMX5_COOP_FALLEN) {
        psx_mod_write_byte(PLAYER+3,0);psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0x80);
    }
    section_pending=stage_departed=0;ever_enrolled=1;last_stage=psx_mod_read_byte(PLAY+0x0C);
    enrolled=1;psx_mod_counter_add("mmx5.coop.enrolled",1);return 1;
}
static uint16_t input(unsigned seat) {
    uint16_t raw=(uint16_t)~sio_get_pad_buttons_slot((int)seat);
    return (uint16_t)((raw<<8)|(raw>>8));
}
static void put32(uint8_t *p,uint32_t value) {
    for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(value>>(i*8));
}
static void start_leave(CPUState *cpu) {
    memcpy(returning_body,second.body,sizeof returning_body);
    memset(second.shots,0,sizeof second.shots);memset(second.trails,0,sizeof second.trails);
    enter_second();guest(cpu,0x8003A2C8,PLAYER,0);guest(cpu,0x8003B33C,PLAYER,3);
    guest(cpu,0x8003B28C,PLAYER,0);psx_mod_write_word(PLAYER+0x54,0);leave_second();
}
static void start_join(CPUState *cpu) {
    memcpy(second.body,returning_body,sizeof returning_body);
    put32(second.body+8,psx_mod_read_word(PLAYER+8));join_y=(int32_t)psx_mod_read_word(PLAYER+12);
    int32_t top=(int16_t)psx_mod_read_half(CAMERA+0x0E)-40;
    put32(second.body+12,(uint32_t)(top*65536));
    put32(second.body+0x18,le32(second.body+8));put32(second.body+0x1C,le32(second.body+12));
    second.body[3]=second.body[4]=1;second.body[5]=second.body[6]=0;
    enter_second();guest(cpu,0x8003A2C8,PLAYER,0);guest(cpu,0x8003B33C,PLAYER,1);
    psx_mod_write_word(PLAYER+0x68,0);psx_mod_write_word(PLAYER+0x54,0);leave_second();
    join_phase=1;
}
static void join_animation(CPUState *cpu) {
    enter_second();guest(cpu,0x80017248,PLAYER,0);
    if(join_phase==1) {
        int32_t y=(int32_t)psx_mod_read_word(PLAYER+12)+8*65536;
        if(y>=join_y) {y=join_y;guest(cpu,0x8003B33C,PLAYER,2);join_phase=2;}
        psx_mod_write_word(PLAYER+12,(uint32_t)y);
    } else if(!psx_mod_read_byte(PLAYER+0x46)) {
        guest(cpu,0x8003A670,PLAYER,0);arrival_grace(cpu);
        join_phase=0;mmx5_coop_teleport_done(&life);
    }
    leave_second();
}
static void retain_survivor(CPUState *cpu) {
    if(life.status[1]!=MMX5_COOP_LEAVING && life.status[1]!=MMX5_COOP_JOINING)return;
    if(life.status[1]==MMX5_COOP_LEAVING)memcpy(second.body,returning_body,sizeof returning_body);
    else put32(second.body+12,(uint32_t)join_y);
    second.body[3]=second.body[4]=1;
    enter_second();guest(cpu,0x8003A2C8,PLAYER,0);guest(cpu,0x8003A670,PLAYER,0);
    arrival_grace(cpu);leave_second();
    life.status[1]=MMX5_COOP_ALIVE;life.select_frames=life.select_armed=0;join_phase=0;
}
static void second_tick(CPUState *cpu,uint32_t address) {
    (void)address;
    if(busy || projected || failed || cpu->gpr[31]!=0x80022DC8u)return;
    if(psx_mod_read_byte(PLAY)!=10 || !psx_mod_read_byte(PLAYER)) {enrolled=0;return;}
    if(scene_owner || psx_mod_read_byte(PLAY+0x10) || psx_mod_read_byte(PLAY+0x1C)) {life.select_frames=0;return;}
    if(!enrolled && (psx_mod_read_byte(PLAYER+4)!=1 || psx_mod_read_byte(PLAYER+5)!=2))return;
    busy=1;
    if(!ready) {ready=load_assets();if(!ready)failed=1;}
    if(ready && !enrolled && !enroll(cpu))failed=1;
    if(enrolled && !failed) {
        uint16_t raw=input(1);
        int safe=!psx_mod_read_byte(PLAYER+0x67) && (psx_mod_read_byte(PLAYER+0x70)&8);
        int gameplay=!psx_mod_read_byte(PLAYER+0xD5) && !second.body[0xD5];
        Mmx5CoopJoinAction action=mmx5_coop_join_input(&life,raw&0x100,gameplay,safe);
        if(action==MMX5_COOP_LEAVE)start_leave(cpu);
        else if(action==MMX5_COOP_JOIN)start_join(cpu);
    }
    if(enrolled && !failed && life.status[1]==MMX5_COOP_JOINING) {
        join_animation(cpu);++ticks;diagnostics();busy=0;return;
    }
    if(enrolled && !failed && life.status[1]!=MMX5_COOP_FALLEN && life.status[1]!=MMX5_COOP_ABSENT &&
       life.status[1]!=MMX5_COOP_UNAVAILABLE) {
        uint8_t saved_input[6];capture(PAD,saved_input,sizeof saved_input);
        uint16_t raw=input(1);enter_second();
        psx_mod_write_half(PAD,life.status[1]==MMX5_COOP_LEAVING?0:raw);psx_mod_write_half(PAD+2,previous_input);
        psx_mod_write_half(PAD+4,(uint16_t)(raw&~previous_input));previous_input=raw;
        guest(cpu,0x8003CDB8,0,0);guest(cpu,0x80035040,0,0);
        guest(cpu,0x80022084,0,0);guest(cpu,0x8002FB58,PLAYER,0);
        leave_second();project(PAD,saved_input,sizeof saved_input);++ticks;
        if(life.status[1]==MMX5_COOP_LEAVING && second.body[4]==3)mmx5_coop_teleport_done(&life);
        if(second.body[4]==2)mmx5_coop_fatal(&life,1);
        if(life.status[1]==MMX5_COOP_DYING && second.body[4]==3)life.status[1]=MMX5_COOP_FALLEN;
        psx_mod_counter_add("mmx5.coop.player-ticks",1);
    }
    diagnostics();busy=0;
}
static int player_tick(CPUState *cpu,uint32_t address) {
    if(player_call || busy || projected || !enrolled || failed)return 0;
    psx_mod_counter_add("mmx5.coop.primary-player-calls",1);
    uint8_t freeze[9];capture(PLAY+0x12,freeze,sizeof freeze);
    player_call=1;uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);player_call=0;
    if(psx_mod_read_byte(PLAYER+4)==2 || life.status[0]==MMX5_COOP_DYING) {
        psx_mod_counter_add("mmx5.coop.primary-death-ticks",1);
        mmx5_coop_fatal(&life,0);
        if(mmx5_coop_living(&life,1))project(PLAY+0x12,freeze,sizeof freeze);
        if(psx_mod_read_byte(PLAYER+4)==3)life.status[0]=MMX5_COOP_FALLEN;
    }
    cpu->gpr[2]=result;return 1;
}
static int stage_tick(CPUState *cpu,uint32_t address) {
    if(stage_call || busy || projected || !enrolled || failed)return 0;
    psx_mod_counter_add("mmx5.coop.stage-dispatch-hooks",1);
    unsigned phase=psx_mod_read_byte(PLAY+1);
    uint16_t raw=input(1);int start=!!(raw&0x0800),pressed=start&&!p2_start_previous;
    p2_start_previous=start;
    if(!phase) {
        pause_owner=-1;
        if(!scene_owner && !psx_mod_read_byte(PLAY+0x1C) && !psx_mod_read_byte(PLAY+0x10) &&
           !psx_mod_read_byte(PLAY+0x0F) && !psx_mod_read_byte(0x8009A424)) {
            if(life.status[0]==MMX5_COOP_ALIVE && (psx_mod_read_half(PAD+4)&0x0800))pause_owner=0;
            else if(life.status[1]==MMX5_COOP_ALIVE && pressed)pause_owner=1;
        }
    } else if(phase==2) {
        life.select_frames=0;
        if(pause_owner<0)pause_owner=mmx5_coop_living(&life,0)?0:1;
    }
    psx_mod_write_byte(diagnostic+0x48,(uint8_t)(pause_owner+1));
    int survivor=life.wipe==2 || (!life.wipe &&
        (life.status[0]==MMX5_COOP_DYING || life.status[0]==MMX5_COOP_FALLEN) &&
        mmx5_coop_living(&life,1));
    if(!survivor && pause_owner!=1 && scene_owner!=2) {
        stage_call=1;uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        enforce_roster();
        if(!psx_mod_read_byte(PLAY+1))scene_tick(cpu);
        if(menu_active && psx_mod_read_byte(PLAY+1)!=2)menu_vram(cpu,0);
        diagnostics();if(psx_mod_read_byte(PLAY)!=10)enrolled=0;
        stage_call=0;cpu->gpr[2]=result;return 1;
    }
    stage_call=1;
    if(pause_owner==1 && !menu_active)menu_vram(cpu,1);
    if(survivor)retain_survivor(cpu);
    /* Advance only P1's death pose before projecting the surviving actor for
     * the one shared native stage pass. Its freeze/timer side effects belong
     * to a team wipe and must not stop a living partner. */
    if(psx_mod_read_byte(PLAY+1)==0 && psx_mod_read_byte(PLAYER+4)==2 && !life.wipe) {
        uint8_t freeze[9];capture(PLAY+0x12,freeze,sizeof freeze);
        uint32_t timer=psx_mod_read_word(PLAY+0x1B4),countdown=psx_mod_read_word(PLAY+0xAC);
        player_call=1;guest(cpu,0x80035040,0,0);player_call=0;
        project(PLAY+0x12,freeze,sizeof freeze);
        psx_mod_write_word(PLAY+0x1B4,timer);psx_mod_write_word(PLAY+0xAC,countdown);
        if(psx_mod_read_byte(PLAYER+4)==3)life.status[0]=MMX5_COOP_FALLEN;
    }
    uint8_t saved_input[6];capture(PAD,saved_input,sizeof saved_input);
    enter_second();
    /* Native story and stage selection remain P1's. Menus alone select the
     * requesting actor's character identity. Player routines use PLAYER+2. */
    if(pause_owner!=1)psx_mod_write_byte(PLAY+0x44,saved_play[0x44]);
    psx_mod_write_half(PAD,raw);psx_mod_write_half(PAD+2,previous_input);
    psx_mod_write_half(PAD+4,(uint16_t)(raw&~previous_input));previous_input=raw;
    world_owner=1;world_packets=arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_BYTES;
    uint32_t world_before=world_ticks;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    if(psx_mod_read_byte(PLAYER+4)==2)mmx5_coop_fatal(&life,1);
    if(life.status[1]==MMX5_COOP_DYING && psx_mod_read_byte(PLAYER+4)==3)life.status[1]=MMX5_COOP_FALLEN;
    world_owner=0;leave_second_context(1);project(PAD,saved_input,sizeof saved_input);
    enforce_roster();
    if(!psx_mod_read_byte(PLAY+1))scene_tick(cpu);
    ticks+=world_ticks-world_before;psx_mod_counter_add("mmx5.coop.survivor-world-ticks",world_ticks-world_before);diagnostics();
    if(menu_active && psx_mod_read_byte(PLAY+1)!=2)menu_vram(cpu,0);
    if(psx_mod_read_byte(PLAY)!=10)enrolled=0;
    stage_call=0;cpu->gpr[2]=result;return 1;
}
static DeathOrigin *death_origin(uint32_t actor) {
    if(actor<0x800D1F40u || actor>=0x800D4F40u || (actor-0x800D1F40u)%0x60)return NULL;
    return &death_origins[(actor-0x800D1F40u)/0x60];
}
static int effect_allocate(CPUState *cpu,uint32_t address) {
    if(effect_allocate_call || !enrolled || failed)return 0;
    uint32_t caller=cpu->gpr[31];effect_allocate_call=1;
    uint32_t actor=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);effect_allocate_call=0;
    DeathOrigin *origin=death_origin(actor);
    if(origin) {
        memset(origin,0,sizeof *origin);
        if(caller==0x8003DABCu || caller==0x8003DB3Cu) {
            origin->x=psx_mod_read_word(PLAYER+8);origin->y=psx_mod_read_word(PLAYER+12);
            origin->layer=psx_mod_read_byte(PLAYER+0x14);
            origin->assembly=psx_mod_read_word(SCRATCH+0x1C);origin->valid=1;
        }
    }
    cpu->gpr[2]=actor;return 1;
}
static int effect_init(CPUState *cpu,uint32_t address) {
    if(effect_init_call || !enrolled || failed)return 0;
    DeathOrigin *origin=death_origin(cpu->gpr[4]);if(!origin || !origin->valid)return 0;
    uint32_t x=psx_mod_read_word(PLAYER+8),y=psx_mod_read_word(PLAYER+12),assembly=psx_mod_read_word(SCRATCH+0x1C);
    uint8_t layer=psx_mod_read_byte(PLAYER+0x14);
    psx_mod_write_word(PLAYER+8,origin->x);psx_mod_write_word(PLAYER+12,origin->y);
    psx_mod_write_byte(PLAYER+0x14,origin->layer);psx_mod_write_word(SCRATCH+0x1C,origin->assembly);
    effect_init_call=1;uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);effect_init_call=0;
    psx_mod_write_word(PLAYER+8,x);psx_mod_write_word(PLAYER+12,y);
    psx_mod_write_byte(PLAYER+0x14,layer);psx_mod_write_word(SCRATCH+0x1C,assembly);
    origin->valid=0;cpu->gpr[2]=result;return 1;
}
static HitHistory *hit_record(uint32_t actor,int create) {
    unsigned i=0;while(i<hit_count && hit_history[i].actor<actor)++i;
    if(i<hit_count && hit_history[i].actor==actor)return &hit_history[i];
    if(!create)return NULL;
    if(hit_count==256) {failed=1;return NULL;}
    memmove(hit_history+i+1,hit_history+i,(hit_count-i)*sizeof hit_history[0]);++hit_count;
    memset(hit_history+i,0,sizeof hit_history[0]);hit_history[i].actor=actor;
    hit_history[i].token[0]=psx_mod_read_byte(actor+0x65);return &hit_history[i];
}
static SolidHistory *solid_record(uint32_t actor) {
    if(actor<0x800C6A10u || actor>=0x800C9310u || (actor-0x800C6A10u)%0xA4)return NULL;
    return &solid_history[(actor-0x800C6A10u)/0xA4];
}
static void forget_actor(uint32_t actor) {
    HitHistory *h=hit_record(actor,0);
    if(h) {unsigned i=(unsigned)(h-hit_history);--hit_count;
        memmove(h,h+1,(hit_count-i)*sizeof *h);memset(hit_history+hit_count,0,sizeof *h);}
    SolidHistory *s=solid_record(actor);if(s)memset(s,0,sizeof *s);
    for(unsigned i=0;i<128;++i)if(pickup_owners[i].actor==actor)pickup_owners[i].actor=0;
}
static int allocate_actor(CPUState *cpu,uint32_t address) {
    if(allocation_call || !enrolled || failed)return 0;
    allocation_call=1;uint32_t actor=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);allocation_call=0;
    if(actor)forget_actor(actor);
    cpu->gpr[2]=actor;return 1;
}
static int contact(CPUState *cpu,uint32_t address) {
    if(contact_call || (projected && !world_owner) || !enrolled || failed || psx_mod_read_byte(PLAY)!=10)return 0;
    contact_call=1;uint32_t actor=cpu->gpr[4],argument=cpu->gpr[5],result=0;
    unsigned seat=world_owner?1:0;
    HitHistory *h=address==0x80031DD0u?hit_record(actor,1):NULL;
    uint8_t stock_token=psx_mod_read_byte(actor+0x65);
    if(h)psx_mod_write_byte(actor+0x65,h->token[seat]);
    if(life.status[seat]==MMX5_COOP_ALIVE && !scene_passenger(seat) && !psx_mod_read_byte(PLAYER+0xCC)) {
        result=guest(cpu,address,actor,argument);
        h=address==0x80031DD0u?hit_record(actor,0):NULL;
        if(h)h->token[seat]=psx_mod_read_byte(actor+0x65);
    }
    if(!world_owner && !result && life.status[1]==MMX5_COOP_ALIVE && !scene_passenger(1) &&
       (second.body[0x5C]&0x7F) && !second.body[0xCC]) {
        if(h)psx_mod_write_byte(actor+0x65,h->token[1]);
        enter_second();result=guest(cpu,address,actor,argument);leave_second();
        h=address==0x80031DD0u?hit_record(actor,0):NULL;
        if(h)h->token[1]=psx_mod_read_byte(actor+0x65);
        if(result) {
            psx_mod_counter_add(address==0x80031DD0u?"mmx5.coop.p2-hits":"mmx5.coop.p2-contact",1);
            psx_mod_write_word(diagnostic+0x68,actor);psx_mod_write_word(diagnostic+0x6C,result);
        }
    }
    if(h)psx_mod_write_byte(actor+0x65,world_owner?stock_token:h->token[0]);
    cpu->gpr[2]=result;contact_call=0;return 1;
}
static uint32_t solid_as_second(CPUState *cpu,uint32_t address,uint32_t actor,SolidHistory *h) {
    uint8_t sides=psx_mod_read_byte(actor+0x72),carried=psx_mod_read_byte(actor+0x76);
    psx_mod_write_byte(actor+0x72,h->sides);psx_mod_write_byte(actor+0x76,h->carried);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    h->sides=psx_mod_read_byte(actor+0x72);h->carried=psx_mod_read_byte(actor+0x76);
    psx_mod_write_byte(actor+0x72,sides);psx_mod_write_byte(actor+0x76,carried);return result;
}
static int solid_contact(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];SolidHistory *h=solid_record(actor);
    if(solid_call || !h || !enrolled || failed || (projected && !world_owner))return 0;
    solid_call=1;uint32_t result;
    if(world_owner)result=solid_as_second(cpu,address,actor,h);
    else {
        result=guest(cpu,address,actor,cpu->gpr[5]);
        if(life.status[1]==MMX5_COOP_ALIVE && !scene_passenger(1) && second.body[4]==1) {
            /* The native function also resolves both shared ride bodies.
             * Their pass ran above; the extra pass handles only P2. */
            uint8_t body[0x158];capture(PLAYER,body,sizeof body);project(PLAYER,second.body,sizeof body);
            uint8_t rides[2]={psx_mod_read_byte(0x800D1DC0),psx_mod_read_byte(0x800D1E70)};
            psx_mod_write_byte(0x800D1DC0,0);psx_mod_write_byte(0x800D1E70,0);
            solid_as_second(cpu,address,actor,h);
            psx_mod_write_byte(0x800D1DC0,rides[0]);psx_mod_write_byte(0x800D1E70,rides[1]);
            capture(PLAYER,second.body,sizeof body);project(PLAYER,body,sizeof body);
        } else memset(h,0,sizeof *h);
    }
    solid_call=0;cpu->gpr[2]=result;return 1;
}
static uint32_t pickup_as_seat(CPUState *cpu,uint32_t address,uint32_t actor,unsigned seat) {
    if(seat==world_owner)return guest(cpu,address,actor,cpu->gpr[5]);
    uint8_t body[0x158],character=psx_mod_read_byte(PLAY+0x44);
    uint8_t *other=seat?second.body:first.body;
    capture(PLAYER,body,sizeof body);project(PLAYER,other,sizeof body);
    psx_mod_write_byte(PLAY+0x44,other[2]);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    capture(PLAYER,other,sizeof body);project(PLAYER,body,sizeof body);
    psx_mod_write_byte(PLAY+0x44,character);return result;
}
static int pickup_collect(CPUState *cpu,uint32_t address) {
    if(pickup_call || !enrolled || failed)return 0;
    pickup_call=1;uint32_t actor=cpu->gpr[4],result=0;
    for(unsigned seat=0;seat<2;++seat) {
        if(life.status[seat]!=MMX5_COOP_ALIVE || scene_passenger(seat))continue;
        result=pickup_as_seat(cpu,address,actor,seat);
        unsigned mode=psx_mod_read_byte(actor+4);if(mode==1)continue;
        if(mode==2) {
            unsigned i=0;while(i<128 && pickup_owners[i].actor && pickup_owners[i].actor!=actor)++i;
            if(i==128) {failed=1;break;}
            pickup_owners[i].actor=actor;pickup_owners[i].seat=(uint8_t)seat;
        }
        psx_mod_write_word(diagnostic+0x70,actor);psx_mod_write_byte(diagnostic+0x74,(uint8_t)(seat+1));break;
    }
    pickup_call=0;cpu->gpr[2]=result;return 1;
}
static int pickup_tick(CPUState *cpu,uint32_t address) {
    if(pickup_call || !enrolled || failed)return 0;
    uint32_t actor=cpu->gpr[4];
    for(unsigned i=0;i<128;++i)if(pickup_owners[i].actor==actor) {
        if(psx_mod_read_byte(actor+4)!=2) {pickup_owners[i].actor=0;return 0;}
        pickup_call=1;uint32_t result=pickup_as_seat(cpu,address,actor,pickup_owners[i].seat);pickup_call=0;
        if(psx_mod_read_byte(actor+4)!=2)pickup_owners[i].actor=0;
        cpu->gpr[2]=result;return 1;
    }
    return 0;
}
static void scene_begin(unsigned seat) {
    if(!enrolled || failed || scene_owner)return;
    scene_owner=seat+1;scene_phase=life.status[seat^1]==MMX5_COOP_ALIVE?1:0;
    life.select_frames=life.select_armed=0;
    psx_mod_counter_add("mmx5.coop.scene-starts",1);
}
static void script_begin(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(!scene_call && (!projected || world_owner))scene_begin(world_owner);
}
static void scene_tick(CPUState *cpu) {
    if(!scene_owner || !enrolled || failed)return;
    unsigned owner=scene_owner-1,passenger=owner^1;
    uint8_t leader[0x158];
    if(owner)memcpy(leader,second.body,sizeof leader);else capture(PLAYER,leader,sizeof leader);
    int locked=leader[0xD0] || leader[0xD4] || leader[0x7A] ||
        psx_mod_read_byte(PLAY+0x10) || psx_mod_read_byte(PLAY+0x1C);
    if(!scene_phase) {
        if(passenger && life.status[1]==MMX5_COOP_LEAVING) {
            unsigned old_busy=busy;busy=1;enter_second();guest(cpu,0x80035EDC,PLAYER,0);leave_second();busy=old_busy;
            if(second.body[4]==3)mmx5_coop_teleport_done(&life);
        }
        if(!locked)scene_owner=0;
        return;
    }
    if(locked && scene_phase>=4)scene_phase=1;
    if(scene_phase==3 && (locked || leader[4]!=1 || leader[5]<2 || leader[0x67] || !(leader[0x70]&8)))return;
    unsigned old_busy=busy;busy=scene_call=1;
    uint8_t play[sizeof saved_play],camera[sizeof saved_camera];
    capture(PLAY,play,sizeof play);capture(CAMERA,camera,sizeof camera);
    if(passenger)enter_second();
    if(scene_phase==1) {
        capture(PLAYER,scene_body,sizeof scene_body);
        for(unsigned i=0;i<sizeof second.shots;++i)psx_mod_write_byte(SHOTS+i,0);
        for(unsigned i=0;i<sizeof second.trails;++i)psx_mod_write_byte(TRAILS+i,0);
        guest(cpu,0x8003A2C8,PLAYER,0);guest(cpu,0x8003B33C,PLAYER,3);guest(cpu,0x8003B28C,PLAYER,0);
        psx_mod_write_word(PLAYER+0x54,0);scene_phase=2;
    } else if(scene_phase==2) {
        guest(cpu,0x80035EDC,PLAYER,0);
        if(psx_mod_read_byte(PLAYER+4)==3)scene_phase=3;
    } else if(scene_phase==3) {
        project(PLAYER,scene_body,sizeof scene_body);scene_y=(int32_t)le32(leader+12);
        uint32_t x=le32(leader+8),y=(uint32_t)(((int16_t)psx_mod_read_half(CAMERA+0x0E)-40)*65536);
        psx_mod_write_word(PLAYER+8,x);psx_mod_write_word(PLAYER+12,y);
        psx_mod_write_word(PLAYER+0x18,x);psx_mod_write_word(PLAYER+0x1C,y);
        psx_mod_write_byte(PLAYER+3,1);psx_mod_write_byte(PLAYER+4,1);
        psx_mod_write_byte(PLAYER+0x14,leader[0x14]);psx_mod_write_byte(PLAYER+0x15,leader[0x15]);
        guest(cpu,0x8003A2C8,PLAYER,0);guest(cpu,0x8003B33C,PLAYER,1);
        psx_mod_write_word(PLAYER+0x68,0);psx_mod_write_word(PLAYER+0x54,0);scene_phase=4;
    } else {
        guest(cpu,0x80017248,PLAYER,0);
        if(scene_phase==4) {
            int32_t y=(int32_t)psx_mod_read_word(PLAYER+12)+8*65536;
            if(y>=scene_y) {y=scene_y;guest(cpu,0x8003B33C,PLAYER,2);scene_phase=5;}
            psx_mod_write_word(PLAYER+12,(uint32_t)y);
        } else if(!psx_mod_read_byte(PLAYER+0x46)) {
            guest(cpu,0x8003A670,PLAYER,0);guest(cpu,0x8002FB58,PLAYER,0);
            arrival_grace(cpu);scene_owner=scene_phase=0;
        }
    }
    if(passenger)leave_second();
    project(PLAY,play,sizeof play);project(CAMERA,camera,sizeof camera);
    scene_call=0;busy=old_busy;
}
static int door_tick(CPUState *cpu,uint32_t address) {
    if(door_call || !enrolled || failed || (projected && !world_owner))return 0;
    door_call=1;uint32_t actor=cpu->gpr[4],result;
    unsigned seat=world_owner;
    if(scene_owner)seat=scene_owner-1;
    else if(!world_owner && life.status[1]==MMX5_COOP_ALIVE) {
        uint32_t hit=life.status[0]==MMX5_COOP_ALIVE?guest(cpu,0x800549E8,actor,0):0;
        if(!hit) {
            enter_second();hit=guest(cpu,0x800549E8,actor,0);leave_second();
            if(hit)seat=1;
        }
    }
    if(seat!=world_owner) {
        enter_second();result=guest(cpu,address,actor,cpu->gpr[5]);
        if(psx_mod_read_byte(PLAYER+0xD4))scene_begin(seat);
        leave_second_context(1);
    } else {
        result=guest(cpu,address,actor,cpu->gpr[5]);
        if(psx_mod_read_byte(PLAYER+0xD4))scene_begin(seat);
    }
    door_call=0;cpu->gpr[2]=result;return 1;
}
static int camera_target(CPUState *cpu,uint32_t address) {
    if(camera_call || busy || projected || !enrolled || failed || scene_owner ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+0x10) ||
       psx_mod_read_byte(PLAY+0x1C) || life.status[1]!=MMX5_COOP_ALIVE ||
       second.body[4]!=1 || psx_mod_read_byte(PLAYER+4)!=1)return 0;
    unsigned axis=address==0x8002A720u?8:12;
    uint32_t original=psx_mod_read_word(PLAYER+axis);
    int32_t target=(int32_t)(((int64_t)(int32_t)original+(int32_t)le32(second.body+axis))/2);
    camera_call=1;psx_mod_write_word(PLAYER+axis,(uint32_t)target);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    psx_mod_write_word(PLAYER+axis,original);camera_call=0;cpu->gpr[2]=result;return 1;
}
static uint32_t actor_word(unsigned seat,unsigned offset) {
    if(seat==world_owner)return psx_mod_read_word(PLAYER+offset);
    return le32((seat?second.body:first.body)+offset);
}
static void actor_word_write(unsigned seat,unsigned offset,uint32_t value) {
    if(seat==world_owner)psx_mod_write_word(PLAYER+offset,value);
    else put32((seat?second.body:first.body)+offset,value);
}
static uint8_t actor_byte(unsigned seat,unsigned offset) {
    return seat==world_owner?psx_mod_read_byte(PLAYER+offset):(seat?second.body:first.body)[offset];
}
static void actor_byte_write(unsigned seat,unsigned offset,uint8_t value) {
    if(seat==world_owner)psx_mod_write_byte(PLAYER+offset,value);
    else (seat?second.body:first.body)[offset]=value;
}
static void team_bounds(void) {
    if(!enrolled || failed || scene_owner || psx_mod_read_byte(PLAY)!=10 ||
       psx_mod_read_byte(PLAY+0x10) || psx_mod_read_byte(PLAY+0x1C))return;
    if(life.status[0]==MMX5_COOP_ALIVE && life.status[1]==MMX5_COOP_ALIVE &&
       !actor_byte(0,0xD5) && !actor_byte(1,0xD5)) {
        int32_t x=(int32_t)actor_word(0,8),other=(int32_t)actor_word(1,8);
        int64_t distance=(int64_t)x-other;
        if(distance>280*65536 || distance < -280*65536) {
            for(unsigned seat=0;seat<2;++seat) {
                int32_t current=seat?other:x,previous=(int32_t)actor_word(seat,0x18);
                int outward=seat?(distance>0?current<previous:current>previous):
                    (distance>0?current>previous:current<previous);
                if(outward)actor_word_write(seat,8,(uint32_t)previous);
            }
        }
    }
    /* Original 80029194..800292E0: authored room bounds, independent of
     * whether the midpoint camera has itself reached the fatal bottom. */
    int left=(int16_t)psx_mod_read_half(CAMERA+0x1E)+8;
    int right=(int16_t)psx_mod_read_half(CAMERA+0x1C)+312;
    int bottom=(int16_t)(psx_mod_read_half(CAMERA+0x20)+256);
    for(unsigned seat=0;seat<2;++seat) {
        if(life.status[seat]!=MMX5_COOP_ALIVE)continue;
        int x=(int16_t)(actor_word(seat,8)>>16),y=(int16_t)(actor_word(seat,12)>>16);
        if(!(psx_mod_read_byte(PLAY+0xC)==5 && actor_byte(seat,0xD5))) {
            if(x<left)actor_word_write(seat,8,(uint32_t)(left*65536));
            if(x>right)actor_word_write(seat,8,(uint32_t)(right*65536));
        }
        unsigned hp=actor_byte(seat,0x5C);
        if(y-8>=bottom && (hp&0x7F)) {
            actor_byte_write(seat,0x5C,0x80);
            psx_mod_write_word(PLAY+0xB8,psx_mod_read_word(PLAY+0xB8)+(hp&0x7F));
        }
    }
}
static int camera_update(CPUState *cpu,uint32_t address) {
    if(camera_update_call || busy || !enrolled || failed)return 0;
    camera_update_call=1;team_bounds();
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    camera_update_call=0;cpu->gpr[2]=result;return 1;
}
static uint16_t sprite_bank(uint32_t table,unsigned frame,uint16_t clut,uint16_t tile) {
    CharacterAssets *a=&assets[counterpart];
    uint8_t native_compressed[65536];
    const uint8_t *compressed;size_t available;uint32_t packed,source;
    if(clut<0x7800)return 0;
    if(render_primary) {
        if(table<0x80010000u || table+(uint32_t)frame*4>=0x80200000u)return 0;
        packed=psx_mod_read_word(table+frame*4);source=table+(packed&0xFFFFFu);
        if(source>=0x80200000u)return 0;
        available=0x80200000u-source;
        if(available>sizeof native_compressed)available=sizeof native_compressed;
        compressed=native_compressed;
    } else {
        if(table<a->sprites || table-a->sprites>=a->graphics_bytes)return 0;
        size_t offset=table-a->sprites,entry=offset+(size_t)frame*4;
        if(entry+4>a->graphics_bytes)return 0;
        packed=le32(a->graphics+entry);source=table+(packed&0xFFFFFu);
        if(source<a->sprites || source-a->sprites>=a->graphics_bytes)return 0;
        compressed=a->graphics+source-a->sprites;available=a->graphics_bytes-(source-a->sprites);
    }
    unsigned tiles=packed>>20;size_t palette_offset=(size_t)(clut-0x7800)*32;
    if(!tiles || tiles>256 || palette_offset+32>0x4000)return 0;
    uint32_t palette=render_primary?psx_mod_read_word(SCRATCH+0x28):a->palette;
    uint8_t colors[32];capture(palette+(uint32_t)palette_offset,colors,sizeof colors);
    for(unsigned i=0;i<bank_count;++i)
        if(banks[i].source==source && banks[i].tile==tile &&
           banks[i].generation==(render_primary?asset_generation:0) &&
           !memcmp(banks[i].colors,colors,sizeof colors))return banks[i].id;
    if(render_primary)capture(source,native_compressed,available);
    uint8_t decoded[32768];uint16_t pixels[256*256];size_t written;
    if(!mmx5_coop_decode_sprite(compressed,
       available,decoded,sizeof decoded,&written) || written!=(size_t)tiles*128)return 0;
    memset(pixels,0,sizeof pixels);unsigned remaining=tiles,row=0,in=0;
    while(remaining) {
        unsigned chunk=remaining<16?remaining:16,width=chunk*16;
        for(unsigned y=0;y<16;++y)for(unsigned x=0;x<width;++x) {
            unsigned index=(decoded[in+(y*width+x)/2]>>((x&1)*4))&15;
            pixels[(row+y)*256+x]=(uint16_t)(colors[index*2]|colors[index*2+1]<<8);
        }
        in+=chunk*128;row+=16;remaining-=chunk;
    }
    uint16_t local[256];uint16_t *output=pixels;unsigned count=65536;
    if(tile!=UINT16_MAX) {
        for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x)
            local[y*16+x]=pixels[(((tile>>8)+y)&255)*256+(((tile&255)+x)&255)];
        output=local;count=256;
    }
    uint32_t hash=2166136261u;
    for(unsigned i=0;i<count;++i)hash=(hash^output[i])*16777619u;
    unsigned index=bank_count;
    for(unsigned i=0;i<bank_count;++i)
        if(banks[i].pixel_hash==hash && banks[i].pixel_count==count &&
           !memcmp(banks[i].pixels,output,count*2)) {index=i;break;}
    if(index==bank_count) {
        if(bank_count==4096)return 0;
        uint16_t *copy=malloc(count*2);if(!copy)return 0;memcpy(copy,output,count*2);
        uint16_t id=(uint16_t)(0x6000+index);
        if(!psx_mod_define_texture_bank(id,tile==UINT16_MAX?256:16,tile==UINT16_MAX?256:16,output)) {
            free(copy);return 0;
        }
        banks[index].id=id;banks[index].pixels=copy;banks[index].pixel_hash=hash;banks[index].pixel_count=count;
        ++bank_count;
    }
    banks[index].source=source;banks[index].tile=tile;
    banks[index].generation=render_primary?asset_generation:0;
    memcpy(banks[index].colors,colors,sizeof colors);return banks[index].id;
}
static void triangle(uint32_t dst,uint32_t next,const uint32_t *q,unsigned a,unsigned b,unsigned c,uint16_t bank) {
    unsigned vertices[3]={a,b,c};psx_mod_write_word(dst,0x09000000u|(next&0xFFFFFFu));
    for(unsigned i=0;i<3;++i) {
        uint32_t tag=i==0?(0x34000000u|(q[1]&0x03000000u)):
            i==1?(uint32_t)(bank&255)<<24:(uint32_t)(bank>>8)<<24;
        uint32_t uv=q[3+vertices[i]*2]&0xFFFF;
        if(i==1)uv|=(0x100u|((q[5]>>16)&0x60u))<<16;
        psx_mod_write_word(dst+4+i*12,tag|(q[1]&0xFFFFFFu));
        psx_mod_write_word(dst+8+i*12,q[2+vertices[i]*2]);psx_mod_write_word(dst+12+i*12,uv);
    }
}
static void render_actor(CPUState *cpu,uint32_t actor,uint32_t base_arena) {
    uint32_t base=psx_mod_read_word(SCRATCH+0x100),mode=psx_mod_read_word(SCRATCH+0x104);
    guest(cpu,0x80024C60,actor,0);uint32_t end=psx_mod_read_word(SCRATCH+0x100);
    if(end<base || end-base_arena>40000 || (end-base)%40) {failed=1;return;}
    uint32_t table=psx_mod_read_word(actor+0x38);unsigned frame=psx_mod_read_byte(actor+0x47);
    const uint8_t *owner=render_primary?first.body:second.body;
    if(!table && psx_mod_read_word(actor+0x3C)==le32(owner+0x3C)) {
        table=le32(owner+0x38);frame=owner[0x47];
    }
    CharacterAssets *a=&assets[counterpart];
    if(!render_primary && (table<a->sprites || table-a->sprites>=a->graphics_bytes))return;
    if(render_primary && (!table || table<0x80010000u || table>=0x80200000u))return;
    uint32_t expanded=base_arena+40000+(base-base_arena)*2;
    for(uint32_t at=base;at<end;at+=40,expanded+=80,mode+=8) {
        uint32_t q[10];for(unsigned i=0;i<10;++i)q[i]=psx_mod_read_word(at+i*4);
        uint16_t tile;int supported=mmx5_coop_sprite_quad(q,&tile);
        if(tile!=UINT16_MAX)q[5]|=(psx_mod_read_word(mode+4)&0x60u)<<16;
        uint16_t bank=supported?sprite_bank(table,frame,(uint16_t)(q[3]>>16),tile):0;
        if(!bank) {failed=1;psx_mod_counter_add("mmx5.coop.sprite-rejected",1);return;}
        triangle(expanded,expanded+40,q,0,1,2,bank);triangle(expanded+40,q[0],q,2,1,3,bank);
        psx_mod_tag_world_primitive(expanded,1);psx_mod_tag_world_primitive(expanded+40,1);
        psx_mod_write_word(at,expanded&0xFFFFFFu);
        /* 8002519C..800251F0 indexes native tails by double buffer and actor
         * priority. Retarget the final tile so another actor cannot unlink it. */
        unsigned priority=psx_mod_read_byte(actor+0x16);
        if(priority!=0xFF) {
            uint32_t tail=0x80091C50u+(psx_mod_read_word(SCRATCH)&1u)*128+
                (priority>>4)*32+(priority&7)*4;
            if(psx_mod_read_word(tail)==at)psx_mod_write_word(tail,expanded+40);
        }
    }
}
static void render(CPUState *cpu,uint32_t address) {
    (void)address;
    if(busy || projected || failed || !enrolled || cpu->gpr[4]!=PLAYER || !second.body[3] ||
       life.status[1]==MMX5_COOP_ABSENT || life.status[1]==MMX5_COOP_FALLEN)return;
    if(psx_mod_read_byte(PLAY)!=10 || !psx_mod_texture_banks_supported())return;
    busy=1;capture_player(&first);project_player(&second);
    uint32_t original=psx_mod_read_word(SCRATCH+0x100),mode=psx_mod_read_word(SCRATCH+0x104);
    uint32_t budget=psx_mod_read_word(SCRATCH+0x124);psx_mod_write_word(SCRATCH+0x124,0);
    uint32_t base=arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_BYTES;
    psx_mod_write_word(SCRATCH+0x100,base);psx_mod_write_word(SCRATCH+0x104,base+0x30000);
    render_actor(cpu,PLAYER,base);
    for(unsigned i=0;i<32 && !failed;++i) {
        uint32_t actor=SHOTS+i*0x9Cu;if(psx_mod_read_byte(actor+3))render_actor(cpu,actor,base);
    }
    psx_mod_write_word(SCRATCH+0x100,original);psx_mod_write_word(SCRATCH+0x104,mode);
    psx_mod_write_word(SCRATCH+0x124,budget);
    project_player(&first);diagnostics();busy=0;
}
static int render_survivor(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!world_owner || !projected || render_call || !enrolled || failed)return 0;
    uint32_t actor=cpu->gpr[4];
    if(actor!=PLAYER && !(actor>=SHOTS && actor<SHOTS+sizeof second.shots))return 0;
    render_call=1;uint32_t original=psx_mod_read_word(SCRATCH+0x100),mode=psx_mod_read_word(SCRATCH+0x104);
    uint32_t base=arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_BYTES;
    capture(PLAYER,second.body,sizeof second.body);
    psx_mod_write_word(SCRATCH+0x100,world_packets);
    psx_mod_write_word(SCRATCH+0x104,base+0x30000+(world_packets-base)/5);
    render_actor(cpu,actor,base);world_packets=psx_mod_read_word(SCRATCH+0x100);
    if(actor==PLAYER && first.body[3] && (life.status[0]==MMX5_COOP_DYING || scene_passenger(0))) {
        /* P2 owns the one world pass, but P1's native death/teleport pose
         * remains visible until its own animation hides the body. */
        uint8_t body[0x158];capture(PLAYER,body,sizeof body);project(PLAYER,first.body,sizeof body);
        uint32_t saved_resources[4];
        for(unsigned i=0;i<4;++i) {
            saved_resources[i]=psx_mod_read_word(SCRATCH+resource_offsets[i]);
            psx_mod_write_word(SCRATCH+resource_offsets[i],resources[i]);
        }
        uint32_t primary_base=primary_arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_BYTES;
        if(primary_packets<primary_base || primary_packets>=primary_base+40000)primary_packets=primary_base;
        psx_mod_write_word(SCRATCH+0x100,primary_packets);
        psx_mod_write_word(SCRATCH+0x104,primary_base+0x30000+(primary_packets-primary_base)/5);
        render_primary=1;render_actor(cpu,PLAYER,primary_base);render_primary=0;
        primary_packets=psx_mod_read_word(SCRATCH+0x100);capture(PLAYER,first.body,sizeof body);
        project(PLAYER,body,sizeof body);
        for(unsigned i=0;i<4;++i)psx_mod_write_word(SCRATCH+resource_offsets[i],saved_resources[i]);
    }
    psx_mod_write_word(SCRATCH+0x100,original);psx_mod_write_word(SCRATCH+0x104,mode);
    render_call=0;return 1;
}
static int render_hook(CPUState *cpu,uint32_t address) {
    if(render_call)return 0;
    if(render_survivor(cpu,address))return 1;
    uint32_t actor=cpu->gpr[4];
    if(busy || projected || failed || !enrolled || psx_mod_read_byte(PLAY)!=10 ||
       (actor!=PLAYER && !(actor>=SHOTS && actor<SHOTS+sizeof second.shots)))return 0;
    uint32_t original=psx_mod_read_word(SCRATCH+0x100),mode=psx_mod_read_word(SCRATCH+0x104);
    uint32_t base=primary_arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_BYTES;
    if(primary_packets<base || primary_packets>=base+40000)primary_packets=base;
    capture(PLAYER,first.body,sizeof first.body);render_call=render_primary=1;
    psx_mod_write_word(SCRATCH+0x100,primary_packets);
    psx_mod_write_word(SCRATCH+0x104,base+0x30000+(primary_packets-base)/5);
    render_actor(cpu,actor,base);primary_packets=psx_mod_read_word(SCRATCH+0x100);
    psx_mod_write_word(SCRATCH+0x100,original);psx_mod_write_word(SCRATCH+0x104,mode);
    render_call=render_primary=0;
    if(actor==PLAYER)render(cpu,address);
    return 1;
}
static uint16_t hud_tile_bank(uint16_t page,uint16_t clut,uint16_t uv) {
    if(page&0x180)return 0;
    uint16_t colors[16],pixels[256];unsigned cx=(clut&63)*16,cy=clut>>6;
    if(cx+16>1024 || cy>=512)return 0;
    /* The native UI owns these CLUTs separately from character sprite colors.
     * The facade synchronizes GL before reading its current palette. */
    for(unsigned i=0;i<16;++i)colors[i]=clut==0x7841?assets[counterpart].hud_colors[i]:
        gpu_vram_peek((int)(cx+i),(int)cy);
    for(unsigned i=0;i<ui_bank_count;++i)
        if(ui_banks[i].character==counterpart && ui_banks[i].page==page && ui_banks[i].clut==clut &&
           ui_banks[i].uv==uv && !memcmp(ui_banks[i].colors,colors,sizeof colors))return ui_banks[i].id;
    if(ui_bank_count==4096)return 0;
    unsigned bx=(page&15)*64,by=(page&16)*16;
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x) {
        unsigned u=((uv&255)+x)&255,v=((uv>>8)+y)&255;
        uint16_t word=assets[counterpart].ui_pixels[(by+v)*1024+bx+u/4];
        pixels[y*16+x]=colors[(word>>((u&3)*4))&15];
    }
    uint16_t id=(uint16_t)(0x7000+ui_bank_count);
    if(!psx_mod_define_texture_bank(id,16,16,pixels))return 0;
    UiBank *bank=&ui_banks[ui_bank_count++];bank->page=page;bank->clut=clut;bank->uv=uv;
    bank->id=id;bank->character=counterpart;memcpy(bank->colors,colors,sizeof colors);return id;
}
static int hud_icon(CPUState *cpu,uint32_t address) {
    (void)address;
    if(hud_seat!=1 || (cpu->gpr[4]!=3 && cpu->gpr[4]!=4))return 0;
    cpu->gpr[2]=0;return 1;
}
static void hud_packets(uint32_t base) {
    uint32_t sprites=psx_mod_read_word(SCRATCH+0x108),modes=psx_mod_read_word(SCRATCH+0x10C);
    uint32_t bars=psx_mod_read_word(SCRATCH+0x110);
    if(sprites<base || sprites>base+0x1000 || (sprites-base)%16 ||
       modes<base+0x1000 || modes>base+0x2000 ||
       (modes-base-0x1000)!=(sprites-base)/2 ||
       bars<base+0x2000 || bars>base+0x3000 || (bars-base-0x2000)%36) {failed=1;return;}
    uint32_t expanded=base+0x3000,mode=base+0x1000;
    for(uint32_t at=base;at<sprites;at+=16,expanded+=80,mode+=8) {
        uint32_t link=psx_mod_read_word(at),uv=psx_mod_read_word(at+12);
        uint16_t page=(uint16_t)psx_mod_read_word(mode+4),bank=hud_tile_bank(page,(uint16_t)(uv>>16),(uint16_t)uv);
        if(!bank || expanded+80>base+0x8000) {failed=1;return;}
        int x=(int16_t)psx_mod_read_half(at+8)+26,y=(int16_t)psx_mod_read_half(at+10);
        uint32_t q[10]={link,0x2C000000u|hud_fade*0x010101u,0,0,0,0,0,0,0,0};
        for(unsigned i=0;i<4;++i) {
            q[2+i*2]=(uint16_t)(x+(i&1)*16)|((uint32_t)(uint16_t)(y+(i>>1)*16)<<16);
            q[3+i*2]=(i&1)*16|((i>>1)*16<<8);
        }
        triangle(expanded,expanded+40,q,0,1,2,bank);triangle(expanded+40,link,q,2,1,3,bank);
        psx_mod_tag_hud_primitive(expanded,-1);psx_mod_tag_hud_primitive(expanded+40,-1);
        psx_mod_write_word(at,expanded&0xFFFFFFu);
    }
    for(uint32_t at=base+0x2000;at<bars;at+=36) {
        for(unsigned xy=8;xy<=32;xy+=8)psx_mod_write_half(at+xy,(uint16_t)(psx_mod_read_half(at+xy)+26));
        for(unsigned c=4;c<=28;c+=8) {
            uint32_t color=psx_mod_read_word(at+c),faded=color&0xFF000000u;
            for(unsigned channel=0;channel<24;channel+=8)faded|=(((color>>channel)&255)*hud_fade/128)<<channel;
            psx_mod_write_word(at+c,faded);
        }
        psx_mod_tag_hud_primitive(at,-1);
    }
}
static int draw_hud(CPUState *cpu,uint32_t address) {
    if(hud_call || !enrolled || failed || psx_mod_read_byte(PLAY)!=10)return 0;
    hud_call=1;
    if(life.status[1]==MMX5_COOP_LEAVING || life.status[1]==MMX5_COOP_ABSENT || life.status[1]==MMX5_COOP_UNAVAILABLE)
        hud_fade=hud_fade>8?hud_fade-8:0;
    else hud_fade=hud_fade<120?hud_fade+8:128;
    uint8_t current[sizeof second.body];capture(PLAYER,current,sizeof current);
    uint32_t assembly_now=psx_mod_read_word(SCRATCH+0x1C),menu_now=psx_mod_read_word(SCRATCH+0x3C);
    if(world_owner) {
        project(PLAYER,first.body,sizeof first.body);
        psx_mod_write_word(SCRATCH+0x1C,resources[1]);psx_mod_write_word(SCRATCH+0x3C,resources[5]);
    }
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    if(world_owner)capture(PLAYER,first.body,sizeof first.body);
    else capture(PLAYER,current,sizeof current);
    if(psx_mod_read_byte(PLAY+0x1F) && hud_fade) {
        project(PLAYER,world_owner?current:second.body,sizeof second.body);
        psx_mod_write_word(SCRATCH+0x1C,assets[counterpart].assembly);
        psx_mod_write_word(SCRATCH+0x3C,assets[counterpart].menu_layout);
        uint32_t pools[3];for(unsigned i=0;i<3;++i)pools[i]=psx_mod_read_word(SCRATCH+0x108+i*4);
        uint32_t base=hud_arena+(psx_mod_read_word(SCRATCH)&1u)*0x8000;
        for(unsigned i=0;i<3;++i)psx_mod_write_word(SCRATCH+0x108+i*4,base+i*0x1000);
        hud_seat=1;guest(cpu,0x800262D8,PLAYER,0);
        guest(cpu,counterpart?0x80025D94:0x80025A74,PLAYER,0);hud_seat=0;hud_packets(base);
        for(unsigned i=0;i<3;++i)psx_mod_write_word(SCRATCH+0x108+i*4,pools[i]);
        if(world_owner)capture(PLAYER,current,sizeof current);
        else capture(PLAYER,second.body,sizeof second.body);
    }
    project(PLAYER,current,sizeof current);psx_mod_write_word(SCRATCH+0x1C,assembly_now);
    psx_mod_write_word(SCRATCH+0x3C,menu_now);hud_call=0;cpu->gpr[2]=result;return 1;
}
static int menu_backup(unsigned x,unsigned y,unsigned w,unsigned h) {
    for(unsigned i=0;i<menu_backup_count;++i) {
        MenuBackup *r=&menu_backups[i];
        if(x>=r->x && y>=r->y && x+w<=r->x+r->w && y+h<=r->y+r->h)return 1;
    }
    if(menu_backup_count==32 || !w || !h || x+w>1024 || y+h>512)return 0;
    MenuBackup *r=&menu_backups[menu_backup_count];r->pixels=malloc((size_t)w*h*2);
    if(!r->pixels)return 0;
    r->x=x;r->y=y;r->w=w;r->h=h;
    (void)gpu_vram_peek((int)x,(int)y);
    const uint16_t *vram=gpu_get_vram();
    for(unsigned row=0;row<h;++row)memcpy(r->pixels+row*w,vram+(y+row)*1024+x,w*2);
    ++menu_backup_count;return 1;
}
static void image_upload(unsigned x,unsigned y,unsigned w,unsigned h,const uint16_t *pixels,unsigned stride) {
    gpu_set_gp0_source(0);gpu_write_gp1(0x04000000u);gpu_write_gp0(0x01000000u);
    gpu_write_gp0(0xA0000000u);gpu_write_gp0(x|(y<<16));gpu_write_gp0(w|(h<<16));
    for(unsigned at=0;at<w*h;at+=2) {
        uint32_t word=pixels[(at/w)*stride+at%w];
        if(at+1<w*h)word|=(uint32_t)pixels[((at+1)/w)*stride+(at+1)%w]<<16;
        gpu_write_gp0(word);
    }
    gpu_write_gp1(0x04000002u);
}
static void menu_vram(CPUState *cpu,int active) {
    if((unsigned)active==menu_active)return;
    guest(cpu,0x80068634,0,0); /* Original DrawSync, identified by SDK body. */
    if(active) {
        menu_active=1;
        static const unsigned rects[2][4]={{320,176,704,80},{832,256,64,256}};
        for(unsigned i=0;i<2;++i) {
            unsigned x=rects[i][0],y=rects[i][1],w=rects[i][2],h=rects[i][3];
            if(!menu_backup(x,y,w,h)) {failed=1;return;}
            image_upload(x,y,w,h,assets[counterpart].ui_pixels+y*1024+x,1024);
        }
    } else {
        for(unsigned i=menu_backup_count;i>0;--i) {
            MenuBackup *r=&menu_backups[i-1];image_upload(r->x,r->y,r->w,r->h,r->pixels,r->w);
            free(r->pixels);r->pixels=NULL;
        }
        menu_backup_count=menu_active=0;
    }
}
static int retained_upload(CPUState *cpu,uint32_t address) {
    (void)address;
    if(upload_call || !enrolled || failed)return 0;
    uint32_t source=cpu->gpr[5],rect=cpu->gpr[4],available=0;
    for(unsigned c=0;c<2;++c)
        if(source>=assets[c].sprites && source-assets[c].sprites<0x58000u)
            available=0x58000u-(source-assets[c].sprites);
    if(!available)return 0;
    unsigned x=psx_mod_read_half(rect),y=psx_mod_read_half(rect+2);
    unsigned w=psx_mod_read_half(rect+4),h=psx_mod_read_half(rect+6);
    uint32_t words=(w*h+1u)/2u;
    if(!w || !h || x+w>1024 || y+h>512 || (source&3u) || words>available/4u) {
        failed=1;cpu->gpr[2]=(uint32_t)-1;return 1;
    }
    upload_call=1;guest(cpu,0x80068634,0,0);
    if(menu_active && !menu_backup(x,y,w,h)) {failed=1;upload_call=0;cpu->gpr[2]=(uint32_t)-1;return 1;}
    gpu_set_gp0_source(source);gpu_write_gp1(0x04000000u);gpu_write_gp0(0x01000000u);
    gpu_write_gp0(0xA0000000u);gpu_write_gp0(x|(y<<16));gpu_write_gp0(w|(h<<16));
    for(uint32_t i=0;i<words;++i) {gpu_set_gp0_source(source+i*4);gpu_write_gp0(psx_mod_read_word(source+i*4));}
    gpu_write_gp1(0x04000002u);upload_call=0;cpu->gpr[2]=0;return 1;
}
static void activate(void) {
    char option[16];keep_zero=!psx_mod_netplay_is_active() &&
        psx_mod_current_option_value("keep_zero",option,sizeof option) && !strcmp(option,"true");
    for(unsigned i=0;i<bank_count;++i) {free(banks[i].pixels);banks[i].pixels=NULL;}
    bank_count=ui_bank_count=0;
    busy=projected=enrolled=failed=ready=ticks=world_ticks=camera_call=0;previous_input=0;
    stage_call=player_call=world_owner=render_call=0;mmx5_coop_lifecycle_init(&life);
    render_primary=0;asset_generation=1;primary_packets=0;
    hud_call=hud_seat=0;hud_fade=128;
    pause_owner=-1;p2_start_previous=0;upload_call=menu_active=0;
    for(unsigned i=0;i<menu_backup_count;++i) {free(menu_backups[i].pixels);menu_backups[i].pixels=NULL;}
    menu_backup_count=0;
    camera_update_call=0;
    ever_enrolled=last_stage=section_pending=stage_departed=0;
    hit_count=allocation_call=solid_call=contact_call=0;
    memset(hit_history,0,sizeof hit_history);memset(solid_history,0,sizeof solid_history);
    pickup_call=0;memset(pickup_owners,0,sizeof pickup_owners);
    scene_owner=scene_phase=scene_call=door_call=0;scene_y=0;memset(scene_body,0,sizeof scene_body);
    join_phase=0;join_y=0;memset(returning_body,0,sizeof returning_body);
    effect_allocate_call=effect_init_call=0;memset(death_origins,0,sizeof death_origins);fixture_sequence=0;
    memset(&first,0,sizeof first);memset(&second,0,sizeof second);
    for(unsigned c=0;c<2;++c) {
        free(assets[c].graphics);free(assets[c].ui_pixels);memset(&assets[c],0,sizeof assets[c]);
        uint32_t memory=psx_mod_alloc_guest_memory(0x58000,16);
        assets[c].sprites=memory;assets[c].assembly=memory+0x40000;
        assets[c].colors=memory+0x4C000;assets[c].palette=memory+0x50000;
        assets[c].menu_colors=memory+0x54000;assets[c].menu_layout=memory+0x54400;
        if(!memory)failed=1;
    }
    arena=psx_mod_alloc_texture_packet_memory(FRAME_BYTES*2,16);
    primary_arena=psx_mod_alloc_texture_packet_memory(FRAME_BYTES*2,16);
    hud_arena=psx_mod_alloc_texture_packet_memory(0x10000,16);
    diagnostic=psx_mod_alloc_guest_memory(0x500,16);if(!arena || !primary_arena || !hud_arena || !diagnostic)failed=1;
    psx_mod_set_rewind_blocked(1);psx_mod_set_savestate_blocked(1);
    psx_mod_counter_add("mmx5.coop.activated",1);
    psx_mod_counter_add("mmx5.coop.diagnostic-address",diagnostic);
}
static uint32_t hash_bytes(uint32_t hash,const uint8_t *data,size_t size) {
    for(size_t i=0;i<size;++i)hash=(hash^data[i])*16777619u;
    return hash;
}
static uint32_t hash_word(uint32_t hash,uint32_t word) {
    for(unsigned i=0;i<4;++i) {hash=(hash^(word&255))*16777619u;word>>=8;}
    return hash;
}
static uint32_t state_digest(void) {
    uint32_t hash=2166136261u;
    const unsigned words[]={ready,enrolled,failed,counterpart,projected,busy,world_owner,keep_zero,
        ever_enrolled,last_stage,section_pending,stage_departed,previous_input,
        life.status[0],life.status[1],life.select_frames,life.select_armed,life.select_previous,life.wipe,
        join_phase,(uint32_t)join_y,(unsigned)(pause_owner+1),(unsigned)p2_start_previous,menu_active,
        scene_owner,scene_phase,(uint32_t)scene_y};
    for(unsigned i=0;i<sizeof words/sizeof words[0];++i)hash=hash_word(hash,words[i]);
    hash=hash_bytes(hash,second.body,sizeof second.body);
    hash=hash_bytes(hash,second.shots,sizeof second.shots);
    hash=hash_bytes(hash,second.trails,sizeof second.trails);
    hash=hash_bytes(hash,returning_body,sizeof returning_body);
    hash=hash_bytes(hash,scene_body,sizeof scene_body);
    if(projected) {
        hash=hash_bytes(hash,first.body,sizeof first.body);
        hash=hash_bytes(hash,first.shots,sizeof first.shots);
        hash=hash_bytes(hash,first.trails,sizeof first.trails);
        hash=hash_bytes(hash,saved_play,sizeof saved_play);hash=hash_bytes(hash,saved_camera,sizeof saved_camera);
        for(unsigned i=0;i<6;++i)hash=hash_word(hash,resources[i]);
    }
    hash=hash_word(hash,hit_count);
    for(unsigned i=0;i<hit_count;++i) {
        hash=hash_word(hash,hit_history[i].actor);hash=hash_bytes(hash,hit_history[i].token,2);
    }
    for(unsigned i=0;i<64;++i) {hash=hash_word(hash,solid_history[i].sides);hash=hash_word(hash,solid_history[i].carried);}
    for(unsigned i=0;i<128;++i) {
        DeathOrigin *o=&death_origins[i];hash=hash_word(hash,o->valid);
        if(o->valid) {hash=hash_word(hash,o->x);hash=hash_word(hash,o->y);
            hash=hash_word(hash,o->assembly);hash=hash_word(hash,o->layer);}
        hash=hash_word(hash,pickup_owners[i].actor);
        if(pickup_owners[i].actor)hash=hash_word(hash,pickup_owners[i].seat);
    }
    hash=hash_word(hash,menu_backup_count);
    for(unsigned i=0;i<menu_backup_count;++i) {
        const MenuBackup *r=&menu_backups[i];hash=hash_word(hash,r->x);hash=hash_word(hash,r->y);
        hash=hash_word(hash,r->w);hash=hash_word(hash,r->h);
        for(unsigned j=0;j<r->w*r->h;++j)hash=hash_word(hash,r->pixels[j]);
    }
    return hash;
}
#ifndef MMX5_COOP_NETPLAY_COMPATIBILITY
#define MMX5_COOP_NETPLAY_COMPATIBILITY "mmx5-coop-delay-unconfigured"
#endif
PSX_MOD_CONSTRUCTOR(mmx5_register_coop) {
    static const PSXModNetplayProfile profile={ID,MMX5_COOP_NETPLAY_COMPATIBILITY,0,0,1,1u,
        NULL,state_digest,1};
    psx_mod_register_netplay_profile(&profile);
    psx_mod_register_activation_plugin(ID,activate);
    psx_mod_register_vblank_plugin(ID,observe_stage);
    psx_mod_register_function_entry_plugin(ID,0x8003BAC8,reset);
    psx_mod_register_function_entry_plugin(ID,0x80022084,second_tick);
    psx_mod_register_function_entry_plugin(ID,0x80022D98,world_tick);
    psx_mod_register_function_filter_plugin(ID,0x80024C60,render_hook);
    psx_mod_register_function_filter_plugin(ID,0x80031DD0,contact);
    psx_mod_register_function_filter_plugin(ID,0x800317B8,contact);
    psx_mod_register_function_filter_plugin(ID,0x8002A720,camera_target);
    psx_mod_register_function_filter_plugin(ID,0x8002A7E0,camera_target);
    psx_mod_register_function_filter_plugin(ID,0x8002A440,camera_update);
    psx_mod_register_function_filter_plugin(ID,0x80035040,player_tick);
    psx_mod_register_function_filter_plugin(ID,0x80020D5C,stage_tick);
    psx_mod_register_function_filter_plugin(ID,0x8002D27C,effect_allocate);
    psx_mod_register_function_filter_plugin(ID,0x8004DB18,effect_init);
    psx_mod_register_function_filter_plugin(ID,0x80025828,draw_hud);
    psx_mod_register_function_filter_plugin(ID,0x80026080,hud_icon);
    psx_mod_register_function_filter_plugin(ID,0x800688E0,retained_upload);
    psx_mod_register_function_filter_plugin(ID,0x8002CA20,allocate_actor);
    psx_mod_register_function_filter_plugin(ID,0x8002C938,allocate_actor);
    psx_mod_register_function_filter_plugin(ID,0x8002D210,allocate_actor);
    psx_mod_register_function_filter_plugin(ID,0x8002D0E4,allocate_actor);
    psx_mod_register_function_filter_plugin(ID,0x8003240C,solid_contact);
    psx_mod_register_function_filter_plugin(ID,0x80054000,pickup_collect);
    psx_mod_register_function_filter_plugin(ID,0x80053A4C,pickup_tick);
    psx_mod_register_function_entry_plugin(ID,0x8003D29C,script_begin);
    psx_mod_register_function_filter_plugin(ID,0x80054BB8,door_tick);
}
#endif /* Development package is staged only with debug tools. */
