/* SLUS-01334 native co-op development implementation.
 * Original 80022D98 runs the shared world once. The hook at its attack-pool
 * boundary adds only the counterpart's input, body, attacks and terrain pass.
 */
#include "mod_plugins.h"
#include "mod_netplay.h"
#include "mod_capcom_bg2d.h"
#include "psx_netplay.h"
#include "cpu_state.h"
#include "sio.h"
#include "gpu.h"
#include "mmx5_coop_assets.h"
#include "mmx5_coop_memory.h"
#include "mmx5_coop_sprite_packet.h"
#include "mmx5_coop_lifecycle.h"
#include "mmx5_coop_views.h"
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
#define CAR 0x800D1DC0u
#define SCRATCH 0x1F800000u
#define FRAME_BYTES 0x40000u
/* Native meter/icon blocks end at Y116. Stack P2 below that block while
 * retaining the original life/ammo geometry, badges, colors and animation. */
#define SECOND_HUD_Y 104

typedef struct {
    uint8_t body[0x158], shots[0x1380], trails[0x120], car[0xB0];
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
static SpriteBank view_banks[4096];
static unsigned view_bank_count;
static unsigned split_views,split_valid,split_owner,split_camera_call,split_scan_call,split_supplemental;
static unsigned split_bounds_call,local_view_call,local_view_seat,local_background_call;
static unsigned spatial_call;
static uint8_t split_cameras[2][0xFC],split_visits[MMX5_COOP_PLACEMENTS/8];
static uint32_t split_table;
static uint32_t view_background_arena;
static unsigned ai_pool_call,ai_call,ai_owner,ai_suspended;
static uint32_t ai_pool,ai_pending,ai_function;
static uint8_t ai_first_freeze;
#define AI_DISPATCH 0x8F7FFF00u
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
static unsigned ui_bank_count,hud_call,hud_seat,hud_icon_call,hud_fade=128;
static UiBank view_ui_banks[4096];
static unsigned view_ui_bank_count;
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
typedef struct {uint8_t sides,carried,car_sides,car_carried;} SolidHistory;
static SolidHistory solid_history[64];
typedef struct {uint32_t actor;uint8_t seat;} PickupOwner;
static PickupOwner pickup_owners[128];
static unsigned pickup_call;
static unsigned scene_owner,scene_phase,scene_call,door_call;
static uint8_t scene_body[0x158];
static int32_t scene_y;
static unsigned keep_zero;
static unsigned ride_call;
static int native_armor_overlay(void);
static uint32_t car_mirror[2];
static uint8_t car_source_owner[272];
static int car_context(void) {
    return psx_mod_read_byte(PLAY+0x0C)==5 && psx_mod_read_word(0x800EE970)==11 &&
        (!psx_mod_read_byte(PLAY+0x1D) || psx_mod_read_byte(PLAYER+0xD5)==0xFF ||
         first.body[0xD5]==0xFF || second.body[0xD5]==0xFF ||
         (psx_mod_read_byte(CAR) && !psx_mod_read_byte(CAR+1)));
}
static int car_source_index(uint32_t actor) {
    if(actor>=0x80092090u && actor<0x80093DD0u && !((actor-0x80092090u)%0x9C))
        return (int)((actor-0x80092090u)/0x9C);
    if(actor>=0x80094398u && actor<0x80096D98u && !((actor-0x80094398u)%0x70))
        return 48+(int)((actor-0x80094398u)/0x70);
    if(actor>=0x800D1F40u && actor<0x800D4F40u && !((actor-0x800D1F40u)%0x60))
        return 144+(int)((actor-0x800D1F40u)/0x60);
    return -1;
}
static void cars_refresh(unsigned default_seat);
static void scene_tick(CPUState *cpu);
static void begin_join(CPUState *cpu,uint32_t x,uint32_t y);
static uint32_t state_digest(void);
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
static void capture(uint32_t address,void *data,size_t bytes) {mmx5_coop_capture(address,data,bytes);}
static void project(uint32_t address,const void *data,size_t bytes) {mmx5_coop_project(address,data,bytes);}
static void capture_player(PlayerContext *p) {
    capture(PLAYER,p->body,sizeof p->body);
    capture(SHOTS,p->shots,sizeof p->shots);
    capture(TRAILS,p->trails,sizeof p->trails);
    if(car_context())capture(CAR,p->car,sizeof p->car);
    else memset(p->car,0,sizeof p->car);
}
static void project_player(const PlayerContext *p) {
    project(PLAYER,p->body,sizeof p->body);
    project(SHOTS,p->shots,sizeof p->shots);
    project(TRAILS,p->trails,sizeof p->trails);
    if(car_context())project(CAR,p->car,sizeof p->car);
}
static void cars_refresh(unsigned default_seat) {
    if(!car_context())return;
    project(car_mirror[0],first.car,sizeof first.car);project(car_mirror[1],second.car,sizeof second.car);
    for(unsigned i=0;i<272;++i) {
        uint32_t actor=i<48?0x80092090u+i*0x9Cu:i<144?0x80094398u+(i-48)*0x70u:
            0x800D1F40u+(i-144)*0x60u;
        if(!psx_mod_read_byte(actor)) {car_source_owner[i]=0;continue;}
        /* Original BIN7 vehicle effects/projectiles store their following
         * actor at +50. Replace only that exact source, preserving hitboxes,
         * assembly, terrain data and unrelated native pointers. */
        if(psx_mod_read_word(actor+0x50)!=CAR)continue;
        if(!car_source_owner[i])car_source_owner[i]=(uint8_t)(default_seat+1);
        psx_mod_write_word(actor+0x50,car_mirror[car_source_owner[i]-1]);
    }
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
    capture_player(&first);cars_refresh(0);projected=1;capture(PLAY,saved_play,sizeof saved_play);
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
    capture_player(&second);cars_refresh(1);project_player(&first);
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
static int ai_canonical_call(CPUState *cpu,uint32_t address,PSXModFunctionFilterCallback callback) {
    if(!ai_call || !ai_owner || ai_suspended)return callback(cpu,address);
    ai_suspended=1;leave_second_context(1);world_owner=0;
    uint32_t result;
    if(callback(cpu,address))result=cpu->gpr[2];
    else result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    uint8_t campaign=psx_mod_read_byte(PLAY+0x44);enter_second();world_owner=1;
    psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
    ai_suspended=0;
    cpu->gpr[2]=result;return 1;
}
static void diagnostics(void) {
    if(!diagnostic)return;
    psx_mod_write_word(diagnostic,ticks);psx_mod_write_word(diagnostic+4,ready);
    psx_mod_write_word(diagnostic+8,enrolled);psx_mod_write_word(diagnostic+12,failed);
    psx_mod_write_word(diagnostic+16,counterpart);psx_mod_write_word(diagnostic+20,bank_count);
    psx_mod_write_word(diagnostic+24,world_ticks);
    psx_mod_write_word(diagnostic+28,psx_mod_read_word(PLAY+0x1B4));
    if(split_views) {
        psx_mod_write_word(diagnostic+0x700,split_valid);
        project(diagnostic+0x710,split_cameras,sizeof split_cameras);
    }
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
    project(diagnostic+0x480,first.car,sizeof first.car);project(diagnostic+0x540,second.car,sizeof second.car);
}
static void reset(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;if(busy || projected)return;
    /* Original 3BBA8/3BBC8..3BCE8 uses signed PLAY+1E == -1 for saved HP/ammo/weapon
     * continuation. A team wipe or campaign departure starts a fresh team. */
    section_pending=ever_enrolled && !stage_departed && !life.wipe &&
        last_stage==psx_mod_read_byte(PLAY+0x0C) && psx_mod_read_byte(PLAY+0x1E)==0xFF;
    enrolled=0;previous_input=0;
    split_valid=split_owner=0;split_table=0;
    memset(split_cameras,0,sizeof split_cameras);memset(split_visits,0,sizeof split_visits);
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
    memset(car_source_owner,0,sizeof car_source_owner);
    ++asset_generation;
    join_phase=0;
    scene_owner=scene_phase=0;
    pause_owner=-1;p2_start_previous=0;
    diagnostics();
}
static void observe_stage(void) {
    unsigned mode=psx_mod_read_byte(PLAY);
    /* Native mode 9 (203F0, including player init at 204E4) belongs to the
     * loader after mode 8. It must not turn a section continuation into a
     * fresh team merely because loading crossed a VBlank. */
    if(ever_enrolled && mode!=8 && mode!=9 && mode!=10)stage_departed=1;
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
                if(car_context()) {
                    /* Original BIN7 EEF98..EEFD8 treats zero car HP as fatal
                     * for that rider. The other seat's car remains native. */
                    if(seat==resident)psx_mod_write_byte(CAR+0x5C,0);
                    else (seat?second.car:first.car)[0x5C]=0;
                }
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
            if(command==7 && native_armor_overlay()) {
                /* Offline controlled mount scenario: place a seat beside an
                 * existing native Armor. Never allocate/clone a vehicle, load
                 * code, alter story flags, or override native mount gates. */
                uint32_t ride=0x800D1DC0u+(psx_mod_read_half(diagnostic+0x58)&1u)*0xB0u;
                if(psx_mod_read_byte(ride) && psx_mod_read_byte(ride+1)==1) {
                    uint32_t x=psx_mod_read_word(ride+8),y=psx_mod_read_word(ride+12)-16*65536u;
                    if(seat==resident) {
                        psx_mod_write_word(PLAYER+8,x);psx_mod_write_word(PLAYER+12,y);
                        psx_mod_write_word(PLAYER+0x18,x);psx_mod_write_word(PLAYER+0x1C,y);
                    } else {
                        uint8_t *body=seat?second.body:first.body;
                        for(unsigned i=0;i<4;++i) {
                            body[8+i]=body[0x18+i]=(uint8_t)(x>>(i*8));
                            body[12+i]=body[0x1C+i]=(uint8_t)(y>>(i*8));
                        }
                    }
                }
            }
            if(command==8) {
                /* Controlled continuation, through the ORIGINAL controller's
                 * negative-exit path (20794..20818). It saves the singleton
                 * native inventory and selects loader mode9 with 1E = FF.
                 * No room/story destination or inventory cache is fabricated. */
                psx_mod_write_byte(PLAY+0x0F,0xFF);guest(cpu,0x800205AC,PLAY,0);
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
    if(car_context() && first.car[0] && !first.car[1] && first.car[4]==1) {
        /* Native arrival selector 3C188..198 uses 3D784 for stage5/checkpoint0.
         * Reuse the already initialized original vehicle without duplicating
         * its global arrival effect or camera setup (BIN7 EEC50..EEE60). */
        project(CAR,first.car,sizeof first.car);
        uint16_t clut=psx_mod_read_half(CAR+0x42);
        psx_mod_write_half(CAR+0x42,(uint16_t)(clut+(counterpart?2:-2)));
        psx_mod_write_word(CAR+8,x+(24u<<16));psx_mod_write_word(CAR+0x18,x+(24u<<16));
        psx_mod_write_byte(CAR+0x5C,psx_mod_read_byte(PLAYER+0x5C));
        uint8_t active=psx_mod_read_byte(CAR);
        guest(cpu,0x8003D784,PLAYER,0);psx_mod_write_byte(CAR,active);
        /* BIN7 EEC50 normally changes the rider's pose and clears +6 so
         * original 377A0 selects its character-specific assembly at 37854.
         * This car is already initialized, so reproduce that rider boundary
         * instead of keeping the old teleport parts with +6 == 1. */
        psx_mod_write_byte(PLAYER+0x15,psx_mod_read_byte(CAR+0x15));
        psx_mod_write_byte(PLAYER+0xD8,first.body[0xD8]);
        psx_mod_write_byte(PLAYER+6,0);
        psx_mod_counter_add("mmx5.coop.chaser-enrolled",1);
    } else {
        guest(cpu,0x8003D5E0,PLAYER,0);guest(cpu,0x8003B65C,PLAYER,0);guest(cpu,0x8003A670,PLAYER,0);
    }
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
    enrolled=1;psx_mod_counter_add("mmx5.coop.enrolled",1);
    /* Like X4/X6, keep initialization private and introduce an absent
     * counterpart with its own original incoming/resolving animation. Native
     * forced vehicle arrival stays separate; retained absent/fallen seats
     * must not be brought back by an ordinary continuation. */
    if(life.status[1]==MMX5_COOP_ALIVE && !car_context()) {
        hud_fade=0;begin_join(cpu,x+(24u<<16),y);
        psx_mod_counter_add("mmx5.coop.stage-arrivals",1);
    }
    return 1;
}
static uint16_t input(unsigned seat) {
    uint16_t raw=(uint16_t)~sio_get_pad_buttons_slot((int)seat);
    return (uint16_t)((raw<<8)|(raw>>8));
}
static int native_armor_overlay(void) {
    /* BIN member 6 (logical ID 10), stage 4/section 1. These VMAs contain
     * unrelated code in other overlays. Never call them on address alone. */
    return psx_mod_read_word(0x800EE970)==10 &&
        psx_mod_read_word(0x800F2A4C)==0x27BDFFE8u &&
        psx_mod_read_word(0x800F2A58)==0x3C02800Au &&
        psx_mod_read_word(0x800EF170)==0x27BDFFE0u;
}
static int ride_tick(CPUState *cpu,uint32_t address) {
    /* Baseline policy: original Ride Armor belongs to P1. Usually the
     * native world already runs under P1 and needs no interception. */
    if(ride_call || busy || !enrolled || failed || !projected || !world_owner || !native_armor_overlay())return 0;
    ride_call=1;
    /* During P2 survivor/script ownership, keep both original world vehicles
     * under P1's body. No clone, alternate mount probe or private vehicle
     * tick: native effects and moving solids retain their stable slot pointers. */
    capture_player(&second);project_player(&first);
    uint32_t pointers[6];
    uint8_t character=psx_mod_read_byte(PLAY+0x44),roster=psx_mod_read_byte(PLAY+0x49);
    psx_mod_write_byte(PLAY+0x44,saved_play[0x44]);psx_mod_write_byte(PLAY+0x49,saved_play[0x49]);
    for(unsigned i=0;i<6;++i) {
        pointers[i]=psx_mod_read_word(SCRATCH+resource_offsets[i]);
        psx_mod_write_word(SCRATCH+resource_offsets[i],resources[i]);
    }
    if(life.status[0]!=MMX5_COOP_ALIVE || scene_passenger(0))
        for(unsigned i=0x7C;i<0x82;++i)psx_mod_write_byte(PLAYER+i,0);
    /* Nested native contacts/scripts must see this as P1's private scope;
     * the outer projected context still belongs to P2 after this call. */
    world_owner=0;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    world_owner=1;
    capture_player(&first);project_player(&second);
    for(unsigned i=0;i<6;++i)psx_mod_write_word(SCRATCH+resource_offsets[i],pointers[i]);
    psx_mod_write_byte(PLAY+0x44,character);psx_mod_write_byte(PLAY+0x49,roster);
    ride_call=0;psx_mod_counter_add("mmx5.coop.p1-armor-world-projections",1);
    cpu->gpr[2]=result;return 1;
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
static void begin_join(CPUState *cpu,uint32_t x,uint32_t y) {
    put32(second.body+8,x);join_y=(int32_t)y;
    int32_t top=(int16_t)psx_mod_read_half(CAMERA+0x0E)-40;
    put32(second.body+12,(uint32_t)(top*65536));
    put32(second.body+0x18,le32(second.body+8));put32(second.body+0x1C,le32(second.body+12));
    second.body[3]=second.body[4]=1;second.body[5]=second.body[6]=0;
    enter_second();guest(cpu,0x8003A2C8,PLAYER,0);guest(cpu,0x8003B33C,PLAYER,1);
    psx_mod_write_word(PLAYER+0x68,0);psx_mod_write_word(PLAYER+0x54,0);leave_second();
    life.status[1]=MMX5_COOP_JOINING;join_phase=1;
}
static void start_join(CPUState *cpu) {
    memcpy(second.body,returning_body,sizeof returning_body);
    begin_join(cpu,psx_mod_read_word(PLAYER+8),psx_mod_read_word(PLAYER+12));
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
    /* The native car arrival drives its rider while PLAY+1C is set. Enroll
     * and advance the second car during that arrival too, so it shares the
     * authored opening instead of joining after the first drop. */
    if(psx_mod_read_byte(PLAY+0x10) ||
       (!car_context() && (scene_owner || psx_mod_read_byte(PLAY+0x1C)))) {life.select_frames=0;return;}
    if(!enrolled && (psx_mod_read_byte(PLAYER+4)!=1 ||
       (psx_mod_read_byte(PLAYER+5)!=2 && !(car_context() && psx_mod_read_byte(PLAYER+5)==0x12))))return;
    /* 3D784 activates the original slot during the player pass, AFTER that
     * frame's vehicle dispatcher. Wait for its next native initialization
     * before copying a car; pose12 alone can still have a zero vehicle body. */
    if(!enrolled && car_context() && psx_mod_read_byte(PLAYER+5)==0x12 &&
       (!psx_mod_read_byte(CAR) || psx_mod_read_byte(CAR+1) || psx_mod_read_byte(CAR+4)!=1))return;
    busy=1;
    if(!ready) {ready=load_assets();if(!ready)failed=1;}
    if(ready && !enrolled && !enroll(cpu))failed=1;
    if(enrolled && !failed) {
        uint16_t raw=input(1);
        int safe=!psx_mod_read_byte(PLAYER+0x67) && (psx_mod_read_byte(PLAYER+0x70)&8);
        int car=mmx5_coop_car_sequence(psx_mod_read_byte(PLAY+0x0C),psx_mod_read_byte(PLAY+0x1D),
            psx_mod_read_byte(0x800D1DC0) && !psx_mod_read_byte(0x800D1DC1));
        if(car && (raw&0x100))psx_mod_counter_add("mmx5.coop.car-departure-blocked",1);
        int gameplay=!car && !psx_mod_read_byte(PLAYER+0xD5) && !second.body[0xD5];
        Mmx5CoopJoinAction action=mmx5_coop_join_input(&life,raw&0x100,gameplay,safe);
        if(action==MMX5_COOP_LEAVE)start_leave(cpu);
        else if(action==MMX5_COOP_JOIN)start_join(cpu);
    }
    if(enrolled && !failed && life.status[1]==MMX5_COOP_JOINING) {
        join_animation(cpu);++ticks;diagnostics();busy=0;return;
    }
    if(enrolled && !failed && life.status[1]!=MMX5_COOP_FALLEN && life.status[1]!=MMX5_COOP_ABSENT &&
       life.status[1]!=MMX5_COOP_UNAVAILABLE) {
        uint8_t saved_input[6],origins[3][20];capture(PAD,saved_input,sizeof saved_input);
        int own_camera=split_views && split_valid && !scene_owner && !car_context();
        if(own_camera)for(unsigned layer=0;layer<3;++layer) {
            capture(CAMERA+layer*0x54u+8,origins[layer],20);
            project(CAMERA+layer*0x54u+8,split_cameras[1]+layer*0x54u+8,20);
        }
        uint16_t raw=input(1);enter_second();
        psx_mod_write_half(PAD,life.status[1]==MMX5_COOP_LEAVING?0:raw);psx_mod_write_half(PAD+2,previous_input);
        psx_mod_write_half(PAD+4,(uint16_t)(raw&~previous_input));previous_input=raw;
        guest(cpu,0x8003CDB8,0,0);
        if(car_context()) {
            uint8_t other=psx_mod_read_byte(CAR+0xB0);psx_mod_write_byte(CAR+0xB0,0);
            guest(cpu,0x80022960,0,0);psx_mod_write_byte(CAR+0xB0,other);
        }
        guest(cpu,0x80035040,0,0);
        guest(cpu,0x80022084,0,0);guest(cpu,0x8002FB58,PLAYER,0);
        leave_second();project(PAD,saved_input,sizeof saved_input);++ticks;
        if(own_camera)for(unsigned layer=0;layer<3;++layer)
            project(CAMERA+layer*0x54u+8,origins[layer],20);
        if(life.status[1]==MMX5_COOP_LEAVING && second.body[4]==3)mmx5_coop_teleport_done(&life);
        if(second.body[4]==2)mmx5_coop_fatal(&life,1);
        if(life.status[1]==MMX5_COOP_DYING && second.body[4]==3)life.status[1]=MMX5_COOP_FALLEN;
        psx_mod_counter_add("mmx5.coop.player-ticks",1);
    }
    diagnostics();busy=0;
}
static int player_tick(CPUState *cpu,uint32_t address) {
    if(player_call || busy || !enrolled || failed)return 0;
    if(projected) {
        if(!world_owner)return 0;
        /* SCRATCH+28 is the entire native stage palette, not just the rider's
         * colors. Keep P2's color changes private during its body pass. */
        uint32_t palette=psx_mod_read_word(SCRATCH+0x28);
        psx_mod_write_word(SCRATCH+0x28,assets[counterpart].palette);
        player_call=1;uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);player_call=0;
        psx_mod_write_word(SCRATCH+0x28,palette);cpu->gpr[2]=result;return 1;
    }
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
static int attacks_tick(CPUState *cpu,uint32_t address) {
    if(!busy && projected && world_owner && enrolled && !failed) {
        uint32_t palette=psx_mod_read_word(SCRATCH+0x28);
        psx_mod_write_word(SCRATCH+0x28,assets[counterpart].palette);
        busy=1;uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);busy=0;
        psx_mod_write_word(SCRATCH+0x28,palette);cpu->gpr[2]=result;return 1;
    }
    second_tick(cpu,address);return 0;
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
        if(car_context()) {capture_player(&first);cars_refresh(0);}
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
        if(car_context())guest(cpu,0x80022960,0,0);
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
    /* Original 56514 caches stage palette pointers in world actors; 2BD8C
     * builds the shared fade from the whole 4000-byte buffer, and 17484
     * uploads it. P2's character-only palette must never own those passes.
     * Body/attack filters above temporarily select its private colors. */
    if(pause_owner!=1)psx_mod_write_word(SCRATCH+0x28,resources[3]);
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
    int source=car_source_index(actor);
    if(source>=0)car_source_owner[source]=(uint8_t)(car_context()?(projected?2:1):0);
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
    int source=car_source_index(actor);
    if(source>=0)car_source_owner[source]=(uint8_t)(car_context()?(projected?2:1):0);
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
static int contact_canonical(CPUState *cpu,uint32_t address) {
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
static int contact(CPUState *cpu,uint32_t address) {return ai_canonical_call(cpu,address,contact_canonical);}
static uint32_t solid_as_second(CPUState *cpu,uint32_t address,uint32_t actor,SolidHistory *h) {
    uint8_t sides=psx_mod_read_byte(actor+0x72),carried=psx_mod_read_byte(actor+0x76);
    uint8_t car_sides=psx_mod_read_byte(actor+0x73),car_carried=psx_mod_read_byte(actor+0x77);
    psx_mod_write_byte(actor+0x72,h->sides);psx_mod_write_byte(actor+0x76,h->carried);
    if(car_context()) {
        psx_mod_write_byte(actor+0x73,h->car_sides);psx_mod_write_byte(actor+0x77,h->car_carried);
    }
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    h->sides=psx_mod_read_byte(actor+0x72);h->carried=psx_mod_read_byte(actor+0x76);
    if(car_context()) {
        h->car_sides=psx_mod_read_byte(actor+0x73);h->car_carried=psx_mod_read_byte(actor+0x77);
        psx_mod_write_byte(actor+0x73,car_sides);psx_mod_write_byte(actor+0x77,car_carried);
    }
    psx_mod_write_byte(actor+0x72,sides);psx_mod_write_byte(actor+0x76,carried);return result;
}
static int solid_contact_canonical(CPUState *cpu,uint32_t address) {
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
            uint8_t car[0xB0];int private_car=car_context();
            if(private_car) {capture(CAR,car,sizeof car);project(CAR,second.car,sizeof second.car);}
            else psx_mod_write_byte(CAR,0);
            psx_mod_write_byte(CAR+0xB0,0);
            solid_as_second(cpu,address,actor,h);
            if(private_car) {capture(CAR,second.car,sizeof second.car);project(CAR,car,sizeof car);}
            else psx_mod_write_byte(CAR,rides[0]);
            psx_mod_write_byte(CAR+0xB0,rides[1]);
            capture(PLAYER,second.body,sizeof body);project(PLAYER,body,sizeof body);
        } else memset(h,0,sizeof *h);
    }
    solid_call=0;cpu->gpr[2]=result;return 1;
}
static int solid_contact(CPUState *cpu,uint32_t address) {return ai_canonical_call(cpu,address,solid_contact_canonical);}
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
static int pickup_collect_canonical(CPUState *cpu,uint32_t address) {
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
static int pickup_collect(CPUState *cpu,uint32_t address) {return ai_canonical_call(cpu,address,pickup_collect_canonical);}
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
    /* Both native cars stop with the one world/script lock. A rider cannot
     * use the ordinary passenger beam while its vehicle remains behind. */
    if(car_context())return;
    if(!enrolled || failed || scene_owner)return;
    if(split_views && split_valid && split_owner!=seat+1) {
        project(CAMERA,split_cameras[seat],sizeof saved_camera);split_owner=seat+1;
    }
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
static int door_tick_canonical(CPUState *cpu,uint32_t address) {
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
    uint8_t canonical_camera[0xFC];unsigned camera_owner=split_owner;
    int scoped_camera=split_views && split_valid && !scene_owner && seat!=world_owner;
    if(scoped_camera) {
        capture(CAMERA,canonical_camera,sizeof canonical_camera);
        project(CAMERA,split_cameras[seat],sizeof canonical_camera);split_owner=seat+1;
    }
    if(seat!=world_owner) {
        enter_second();result=guest(cpu,address,actor,cpu->gpr[5]);
        if(psx_mod_read_byte(PLAYER+0xD4))scene_begin(seat);
        leave_second_context(1);
    } else {
        result=guest(cpu,address,actor,cpu->gpr[5]);
        if(psx_mod_read_byte(PLAYER+0xD4))scene_begin(seat);
    }
    if(scoped_camera) {
        capture(CAMERA,split_cameras[seat],sizeof canonical_camera);
        if(scene_owner!=seat+1) {project(CAMERA,canonical_camera,sizeof canonical_camera);split_owner=camera_owner;}
    }
    door_call=0;cpu->gpr[2]=result;return 1;
}
static int door_tick(CPUState *cpu,uint32_t address) {return ai_canonical_call(cpu,address,door_tick_canonical);}
static int camera_target(CPUState *cpu,uint32_t address) {
    /* BIN7 EF05C..EF098 carries a car with the authored camera delta. A
     * midpoint target adds feedback into that forced scroll; keep its original
     * controller while both riders are present and let a survivor own it. */
    if(split_views || camera_call || busy || projected || !enrolled || failed || scene_owner || car_context() ||
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
    if(!split_views && life.status[0]==MMX5_COOP_ALIVE && life.status[1]==MMX5_COOP_ALIVE &&
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
        if(split_views && split_valid && !car_context()) {
            const uint8_t *view=split_cameras[seat];
            left=(int16_t)(view[0x1E]|view[0x1F]<<8)+8;
            right=(int16_t)(view[0x1C]|view[0x1D]<<8)+312;
            bottom=(int16_t)((uint16_t)(view[0x20]|view[0x21]<<8)+256);
        }
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
    if(camera_update_call || split_camera_call || psx_mod_local_view_scope() || busy || !enrolled || failed)return 0;
    if(split_views && split_valid && split_owner!=world_owner+1 && !scene_owner && !car_context())
        project(CAMERA,split_cameras[world_owner],sizeof saved_camera);
    camera_update_call=1;team_bounds();
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    camera_update_call=0;cpu->gpr[2]=result;return 1;
}
static int split_available(void) {
    return split_views && split_valid && enrolled && !failed &&
        psx_mod_read_byte(PLAY)==10 && !psx_mod_local_view_scope();
}
static void split_actors(Mmx5CoopViewActor actors[2]) {
    unsigned resident=projected?1u:0u;
    for(unsigned seat=0;seat<2;++seat) {
        const uint8_t *body=seat?second.body:first.body;
        actors[seat]=(Mmx5CoopViewActor){seat==resident?(int16_t)psx_mod_read_half(PLAYER+10):
            (int16_t)(body[10]|body[11]<<8),seat==resident?(int16_t)psx_mod_read_half(PLAYER+14):
            (int16_t)(body[14]|body[15]<<8),(unsigned)mmx5_coop_living(&life,seat)};
    }
}
static void ai_redirect(CPUState *cpu,uint32_t address) {
    if(!ai_pool_call || ai_call || projected || !split_available())return;
    uint32_t base=address==0x80022044u?0x80092090u:0x80096D98u;
    unsigned capacity=address==0x80022044u?48u:32u;
    uint32_t actor=cpu->gpr[4],function=cpu->gpr[5];
    if(actor<base || actor>=base+capacity*0x9Cu || (actor-base)%0x9Cu || ai_pending ||
       (function&3u) || function<0x80010000u || function>=0x80200000u)return;
    ai_pending=actor;ai_function=function;cpu->gpr[5]=AI_DISPATCH;
}
static void ai_update(CPUState *cpu,uint32_t address) {
    (void)address;uint32_t actor=ai_pending,function=ai_function;
    ai_pending=ai_function=0;if(!actor || !function || ai_call || !ai_pool_call)return;
    psx_mod_write_byte(PLAYER+0xCC,ai_first_freeze);
    Mmx5CoopViewActor actors[2];split_actors(actors);
    unsigned target=scene_owner?scene_owner-1:mmx5_coop_view_nearest(actors,
        (int16_t)psx_mod_read_half(actor+10),(int16_t)psx_mod_read_half(actor+14),0);
    ai_call=1;ai_owner=target;uint32_t result=0;
    if(actors[target].active) {
        /* Native grabs/status changes can call player-part helpers too.
         * Select the matching body AND character resources, preserving the
         * whole-stage palette and campaign. Nested contacts restore the
         * normal two-player ownership boundary before resuming this scope. */
        if(target) {
            uint8_t campaign=psx_mod_read_byte(PLAY+0x44);enter_second();world_owner=1;
            psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
        }
        if(psx_mod_read_byte(PLAYER+0xCC)) {
            if(psx_mod_read_byte(actor+3))result=guest(cpu,0x8002DAA0,actor,0);
        } else {
            result=guest(cpu,function,actor,function);
            psx_mod_counter_add(target?"mmx5.coop.p2-native-ai":"mmx5.coop.p1-native-ai",1);
        }
        if(target) {leave_second_context(1);world_owner=0;}
    }
    ai_first_freeze=psx_mod_read_byte(PLAYER+0xCC);psx_mod_write_byte(PLAYER+0xCC,0);
    ai_call=ai_owner=0;cpu->gpr[2]=result;
}
static int ai_pool_tick(CPUState *cpu,uint32_t address) {
    if(ai_pool_call || projected || car_context() || !split_available())return 0;
    ai_pool_call=1;ai_pool=address;ai_first_freeze=psx_mod_read_byte(PLAYER+0xCC);
    psx_mod_write_byte(PLAYER+0xCC,0);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    psx_mod_write_byte(PLAYER+0xCC,ai_first_freeze);ai_first_freeze=0;
    ai_pool_call=0;ai_pool=0;cpu->gpr[2]=result;return 1;
}
static unsigned split_view_seat(unsigned seat) {
    if(scene_owner)return scene_owner-1;
    if(seat>1)seat=0;
    if(!mmx5_coop_living(&life,seat))return mmx5_coop_living(&life,seat^1)?seat^1:0;
    return seat;
}
static int split_camera_finish(CPUState *cpu,uint32_t address) {
    if(split_camera_call || psx_mod_local_view_scope() || !split_views || !enrolled || failed)return 0;
    split_camera_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    uint8_t canonical[0xFC];capture(CAMERA,canonical,sizeof canonical);
    unsigned owner=world_owner,other=owner^1;
    if(!split_valid) {
        memcpy(split_cameras[0],canonical,sizeof canonical);memcpy(split_cameras[1],canonical,sizeof canonical);
        split_valid=1;
    }
    memcpy(split_cameras[owner],canonical,sizeof canonical);split_owner=owner+1;
    if(scene_owner || car_context() || psx_mod_read_byte(PLAY+0x10) ||
       psx_mod_read_byte(PLAY+0x1C) || !mmx5_coop_living(&life,other)) {
        memcpy(split_cameras[other],canonical,sizeof canonical);
    } else if(!projected) {
        /* Original primary/parallax updates run once for this extra camera,
         * restoring the canonical camera and all private player projection. */
        memcpy(split_cameras[other],canonical,8);
        project(CAMERA,split_cameras[other],sizeof canonical);
        enter_second();
        guest(cpu,0x8002A440,0,0);guest(cpu,0x8002A4E4,0,0);guest(cpu,0x8002A554,0,0);
        capture(CAMERA,split_cameras[other],sizeof canonical);leave_second();
        project(CAMERA,canonical,sizeof canonical);
        psx_mod_counter_add("mmx5.coop.split-camera-ticks",1);
    }
    split_camera_call=0;diagnostics();cpu->gpr[2]=result;return 1;
}
static unsigned split_regions(PSXPlacementRect regions[2]) {
    Mmx5CoopViewActor actors[2];split_actors(actors);int16_t cameras[2][2];
    for(unsigned seat=0;seat<2;++seat) {
        cameras[seat][0]=(int16_t)(split_cameras[seat][10]|split_cameras[seat][11]<<8);
        cameras[seat][1]=(int16_t)(split_cameras[seat][14]|split_cameras[seat][15]<<8);
    }
    return mmx5_coop_view_regions(cameras,actors,scene_owner?(int)scene_owner-1:-1,regions);
}
static int split_placement_index(uint32_t record,unsigned *index) {
    if(!split_table || record<split_table || (record-split_table)%8 ||
       record-split_table>=MMX5_COOP_PLACEMENTS*8u)return 0;
    *index=(record-split_table)/8u;return 1;
}
static int split_allocate(CPUState *cpu,uint32_t address) {
    (void)address;unsigned index;
    if(!split_available() || cpu->gpr[31]!=0x8002B280 ||
       !split_placement_index(cpu->gpr[18],&index))return 0;
    uint32_t record=cpu->gpr[18];unsigned category=psx_mod_read_byte(record+3)&15;
    if((split_visits[index/8]&(1u<<(index&7))) ||
       (split_supplemental && ((psx_mod_read_byte(record)&0x70) || category>=3))) {
        cpu->gpr[2]=0;psx_mod_counter_add("mmx5.coop.split-respawn-suppressed",1);return 1;
    }
    return 0;
}
static void split_spawned(CPUState *cpu,uint32_t address) {
    (void)address;unsigned index;
    /* Original successful allocation (+s1 != 0), never an attempted spawn. */
    if(split_available() && cpu->gpr[17] && split_placement_index(cpu->gpr[18],&index))
        split_visits[index/8]|=(uint8_t)(1u<<(index&7));
}
static void split_scan_rectangle(CPUState *cpu,PSXPlacementRect region) {
    CPUState saved=*cpu;cpu->gpr[29]-=32;
    uint32_t arg=cpu->gpr[29]+16,previous=psx_mod_read_word(arg);
    cpu->gpr[4]=(uint32_t)region.left;cpu->gpr[5]=(uint32_t)region.right;
    cpu->gpr[6]=(uint32_t)region.top;cpu->gpr[7]=(uint32_t)region.bottom;
    psx_mod_write_word(arg,0);cpu->gpr[31]=0x8002B570;
    psx_dispatch_call(cpu,0x8002B070,cpu->gpr[31]);
    psx_mod_write_word(arg,previous);
    if(cpu->gte_ts_done>saved.gte_ts_done)saved.gte_ts_done=cpu->gte_ts_done;
    if(cpu->muldiv_ts_done>saved.muldiv_ts_done)saved.muldiv_ts_done=cpu->muldiv_ts_done;
    *cpu=saved;
}
static int split_scan(CPUState *cpu,uint32_t address) {
    if(split_scan_call || !split_available())return 0;
    unsigned stage=psx_mod_read_byte(PLAY+12),section=psx_mod_read_byte(PLAY+13);
    if(stage>=19 || section>1)return 0;
    uint32_t table=psx_mod_read_word(0x80072EACu+stage*8u+section*4u);
    if((table&7u) || table<0x80010000u || table>=0x80200000u)return 0;
    if(table!=split_table) {split_table=table;memset(split_visits,0,sizeof split_visits);}
    PSXPlacementRect regions[2];unsigned count=split_regions(regions);
    for(unsigned index=0;index<MMX5_COOP_PLACEMENTS && table+index*8u+8u<=0x80200000u;++index) {
        uint32_t record=table+index*8u;
        if(psx_mod_read_byte(record+3)==15)break;
        unsigned mask=1u<<(index&7);
        int inside=mmx5_coop_view_contains(regions,count,(int16_t)psx_mod_read_half(record+4),
                                         (int16_t)psx_mod_read_half(record+6));
        if(!inside)split_visits[index/8]&=(uint8_t)~mask;
        else if(psx_mod_read_byte(record)&1)split_visits[index/8]|=(uint8_t)mask;
    }
    uint8_t canonical[0xFC];capture(CAMERA,canonical,sizeof canonical);split_scan_call=1;
    Mmx5CoopViewActor actors[2];split_actors(actors);uint32_t result=0;
    for(unsigned seat=0;seat<2;++seat)if(actors[seat].active && (!scene_owner || scene_owner==seat+1)) {
        project(CAMERA,split_cameras[seat],sizeof canonical);
        result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    }
    split_supplemental=1;
    for(unsigned index=0;index<count;++index) {
        /* 2B070 also uses its camera for native cooldown re-entry bounds. */
        unsigned seat=scene_owner?scene_owner-1:index;
        if(count==1 && !actors[0].active)seat=1;
        project(CAMERA,split_cameras[seat],sizeof canonical);
        split_scan_rectangle(cpu,regions[index]);
    }
    project(CAMERA,canonical,sizeof canonical);split_scan_call=split_supplemental=0;
    psx_mod_counter_add("mmx5.coop.split-placement-scans",count);cpu->gpr[2]=result;return 1;
}
static int split_actor_bounds(CPUState *cpu,uint32_t address) {
    if(split_bounds_call || !split_available())return 0;
    uint32_t actor=cpu->gpr[4];unsigned layer=psx_mod_read_byte(actor+0x14);
    if(layer>=3)return 0;
    int draw=address!=0x8002D800u && address!=0x8002D938u;
    uint8_t canonical[0xFC];capture(CAMERA,canonical,sizeof canonical);
    Mmx5CoopViewActor actors[2];split_actors(actors);
    unsigned visible=0;uint32_t result=draw?0:1;split_bounds_call=1;
    for(unsigned seat=0;seat<2;++seat)if(actors[seat].active && (!scene_owner || scene_owner==seat+1)) {
        project(CAMERA,split_cameras[seat],sizeof canonical);
        uint32_t candidate=guest(cpu,address,actor,cpu->gpr[5]);
        if(draw) {if(psx_mod_read_byte(actor+3)) {visible=1;result=candidate;}}
        else if(!candidate)result=0;
    }
    if(draw)psx_mod_write_byte(actor+3,(uint8_t)visible);
    project(CAMERA,canonical,sizeof canonical);split_bounds_call=0;cpu->gpr[2]=result;return 1;
}
static int camera_area_contains(uint32_t parameters,Mmx5CoopViewActor actor) {
    return actor.active && parameters>=0x80010000u && parameters<=0x801FFFF0u &&
        actor.x>=(int)psx_mod_read_half(parameters) && actor.x<=(int)psx_mod_read_half(parameters+2) &&
        actor.y>=(int)psx_mod_read_half(parameters+4) && actor.y<=(int)psx_mod_read_half(parameters+6);
}
static void camera_area_apply(uint32_t parameters,uint8_t *view) {
    /* Original 55F60 uses a fixed sixteen-byte record, not X4's variable
     * assignment lists. FFFF preserves that bound; +47 is native speed2. */
    const unsigned targets[4]={0x26,0x24,0x2A,0x28};view[0x47]=2;
    for(unsigned index=0;index<4;++index) {
        uint16_t value=psx_mod_read_half(parameters+8+index*2);
        if(value!=0xFFFF) {view[targets[index]]=(uint8_t)value;view[targets[index]+1]=(uint8_t)(value>>8);}
    }
}
static int spatial_stage_actor(CPUState *cpu,uint32_t address) {
    if(spatial_call || projected || car_context() || !split_available())return 0;
    uint32_t actor=cpu->gpr[4];
    if(actor<0x8009E7A0u || actor>=0x8009F9A0u || (actor-0x8009E7A0u)%0x30u)return 0;
    unsigned checkpoint=address==0x80056AA0u;
    if(psx_mod_read_byte(actor+1)!=(checkpoint?6u:0u))return 0;
    Mmx5CoopViewActor actors[2];split_actors(actors);Mmx5CoopViewActor eligible[2]={actors[0],actors[1]};
    uint32_t parameters=psx_mod_read_byte(actor+4)?psx_mod_read_word(actor+0x18):0;
    for(unsigned seat=0;seat<2;++seat) {
        if(!checkpoint && parameters)eligible[seat].active=(unsigned)camera_area_contains(parameters,actors[seat]);
        if(checkpoint)eligible[seat].active=actors[seat].active &&
            ((psx_mod_read_byte(actor+2)&0x80)?actors[seat].y>=(int16_t)psx_mod_read_half(actor+14):
                                             actors[seat].x>=(int16_t)psx_mod_read_half(actor+10));
    }
    unsigned target=scene_owner?scene_owner-1:mmx5_coop_view_nearest(
        eligible[0].active || eligible[1].active?eligible:actors,
        (int16_t)psx_mod_read_half(actor+10),(int16_t)psx_mod_read_half(actor+14),0);
    uint8_t canonical[0xFC];capture(CAMERA,canonical,sizeof canonical);spatial_call=1;
    if(target) {project(CAMERA,split_cameras[target],sizeof canonical);enter_second();}
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    if(target) {
        capture(CAMERA,split_cameras[target],sizeof canonical);leave_second_context(1);
        project(CAMERA,canonical,sizeof canonical);
    }
    if(!checkpoint && !scene_owner && !psx_mod_read_byte(PLAY+0x10) && !psx_mod_read_byte(PLAY+0x1C)) {
        parameters=psx_mod_read_word(actor+0x18);
        for(unsigned seat=0;seat<2;++seat)if(camera_area_contains(parameters,actors[seat])) {
            camera_area_apply(parameters,split_cameras[seat]);
            if(!seat) {
                uint8_t current[0xFC];capture(CAMERA,current,sizeof current);camera_area_apply(parameters,current);
                project(CAMERA,current,sizeof current);
            }
        }
    }
    spatial_call=0;psx_mod_counter_add(target?"mmx5.coop.p2-spatial-stage":"mmx5.coop.p1-spatial-stage",1);
    cpu->gpr[2]=result;return 1;
}
static uint16_t sprite_bank(uint32_t table,unsigned frame,uint16_t clut,uint16_t tile) {
    unsigned *used=psx_mod_local_view_scope()?&view_bank_count:&bank_count;
    SpriteBank *cache=psx_mod_local_view_scope()?view_banks:banks;
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
    for(unsigned i=0;i<(*used);++i)
        if(cache[i].source==source && cache[i].tile==tile &&
           cache[i].generation==(render_primary?asset_generation:0) &&
           !memcmp(cache[i].colors,colors,sizeof colors))return cache[i].id;
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
    unsigned index=(*used);
    for(unsigned i=0;i<(*used);++i)
        if(cache[i].pixel_hash==hash && cache[i].pixel_count==count &&
           !memcmp(cache[i].pixels,output,count*2)) {index=i;break;}
    if(index==(*used)) {
        if((*used)==4096)return 0;
        uint16_t *copy=malloc(count*2);if(!copy)return 0;memcpy(copy,output,count*2);
        uint16_t id=(uint16_t)((psx_mod_local_view_scope()?0xA000u:0x6000u)+index);
        if(!psx_mod_define_texture_bank(id,tile==UINT16_MAX?256:16,tile==UINT16_MAX?256:16,output)) {
            free(copy);return 0;
        }
        cache[index].id=id;cache[index].pixels=copy;cache[index].pixel_hash=hash;cache[index].pixel_count=count;
        ++(*used);
    }
    cache[index].source=source;cache[index].tile=tile;
    cache[index].generation=render_primary?asset_generation:0;
    memcpy(cache[index].colors,colors,sizeof colors);return cache[index].id;
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
    /* Stage vehicle graphics remain the original shared stage atlas. They
     * are not character DAT frames and do not need a retained character bank. */
    if(car_context() && second.car[3])guest(cpu,0x80024C60,CAR,0);
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
    unsigned *used=psx_mod_local_view_scope()?&view_ui_bank_count:&ui_bank_count;
    UiBank *cache=psx_mod_local_view_scope()?view_ui_banks:ui_banks;
    if(page&0x180)return 0;
    uint16_t colors[16],pixels[256];unsigned cx=(clut&63)*16,cy=clut>>6;
    if(cx+16>1024 || cy>=512)return 0;
    /* The native UI owns these CLUTs separately from character sprite colors.
     * The facade synchronizes GL before reading its current palette. */
    for(unsigned i=0;i<16;++i)colors[i]=clut==0x7841?assets[counterpart].hud_colors[i]:
        gpu_vram_peek((int)(cx+i),(int)cy);
    for(unsigned i=0;i<(*used);++i)
        if(cache[i].character==counterpart && cache[i].page==page && cache[i].clut==clut &&
           cache[i].uv==uv && !memcmp(cache[i].colors,colors,sizeof colors))return cache[i].id;
    if((*used)==4096)return 0;
    unsigned bx=(page&15)*64,by=(page&16)*16;
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x) {
        unsigned u=((uv&255)+x)&255,v=((uv>>8)+y)&255;
        uint16_t word=assets[counterpart].ui_pixels[(by+v)*1024+bx+u/4];
        pixels[y*16+x]=colors[(word>>((u&3)*4))&15];
    }
    uint16_t id=(uint16_t)((psx_mod_local_view_scope()?0xB000u:0x7000u)+(*used));
    if(!psx_mod_define_texture_bank(id,16,16,pixels))return 0;
    UiBank *bank=&cache[(*used)++];bank->page=page;bank->clut=clut;bank->uv=uv;
    bank->id=id;bank->character=counterpart;memcpy(bank->colors,colors,sizeof colors);return id;
}
static int hud_icon(CPUState *cpu,uint32_t address) {
    if(!psx_mod_local_view_scope() && hud_seat==1 && (cpu->gpr[4]==3 || cpu->gpr[4]==4)) {cpu->gpr[2]=0;return 1;}
    if(psx_mod_local_view_scope())return 0;
    if(hud_icon_call || !hud_call || hud_seat || cpu->gpr[4]!=10)return 0;
    /* Native virus/radar block occupies the lower left in Maverick stages.
     * Move its existing sprites together so the stacked P2 meter stays clear. */
    hud_icon_call=1;uint32_t start=psx_mod_read_word(SCRATCH+0x108);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    uint32_t end=psx_mod_read_word(SCRATCH+0x108);
    if(end>=start && end-start<=0x1000 && !((end-start)%16))
        for(uint32_t at=start;at<end;at+=16)psx_mod_write_half(at+8,(uint16_t)(psx_mod_read_half(at+8)+48));
    hud_icon_call=0;cpu->gpr[2]=result;return 1;
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
        int offset=psx_mod_local_view_scope()?0:SECOND_HUD_Y;
        int x=(int16_t)psx_mod_read_half(at+8),y=(int16_t)psx_mod_read_half(at+10)+offset;
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
        int offset=psx_mod_local_view_scope()?0:SECOND_HUD_Y;
        for(unsigned xy=10;xy<=34;xy+=8)psx_mod_write_half(at+xy,(uint16_t)(psx_mod_read_half(at+xy)+offset));
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
    if(split_views && psx_mod_local_view_scope()) {
        hud_call=1;uint32_t result;
        if(!local_view_seat)result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        else {
            uint8_t current[sizeof second.body];capture(PLAYER,current,sizeof current);
            uint32_t assembly_now=psx_mod_read_word(SCRATCH+0x1C),menu_now=psx_mod_read_word(SCRATCH+0x3C);
            uint8_t character=psx_mod_read_byte(PLAY+0x44);
            project(PLAYER,second.body,sizeof second.body);
            psx_mod_write_byte(PLAY+0x44,(uint8_t)counterpart);
            psx_mod_write_word(SCRATCH+0x1C,assets[counterpart].assembly);
            psx_mod_write_word(SCRATCH+0x3C,assets[counterpart].menu_layout);
            uint32_t pools[3];for(unsigned i=0;i<3;++i)pools[i]=psx_mod_read_word(SCRATCH+0x108+i*4);
            uint32_t base=hud_arena+(psx_mod_read_word(SCRATCH)&1u)*0x8000;
            for(unsigned i=0;i<3;++i)psx_mod_write_word(SCRATCH+0x108+i*4,base+i*0x1000);
            hud_fade=128;hud_seat=1;result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
            hud_seat=0;hud_packets(base);
            for(unsigned i=0;i<3;++i)psx_mod_write_word(SCRATCH+0x108+i*4,pools[i]);
            /* Scene drawing still uses the canonical P1 body/resources. */
            project(PLAYER,current,sizeof current);psx_mod_write_byte(PLAY+0x44,character);
            psx_mod_write_word(SCRATCH+0x1C,assembly_now);psx_mod_write_word(SCRATCH+0x3C,menu_now);
        }
        hud_call=0;cpu->gpr[2]=result;return 1;
    }
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
typedef struct {
    PlayerContext first,second;
    uint8_t play[0x1C0],camera[0xFC],car_owners[272];
    uint32_t resources[6],world_packets,primary_packets;
    unsigned projected,busy,world_owner,render_call,render_primary,hud_call,hud_seat,hud_fade,upload_call,failed;
} LocalViewContext;
static int draw_local_view(CPUState *cpu,void *user,uint32_t alpha) {
    (void)alpha;uint32_t buffer=*(uint32_t *)user;
    project(CAMERA,split_cameras[local_view_seat],sizeof saved_camera);
    gpu_set_gp0_source(0);
    gpu_write_gp0(0x02000000u|psx_mod_read_byte(buffer+0x2D)|
        (uint32_t)psx_mod_read_byte(buffer+0x2E)<<8|(uint32_t)psx_mod_read_byte(buffer+0x2F)<<16);
    gpu_write_gp0(psx_mod_read_half(buffer+0x14)|(uint32_t)psx_mod_read_half(buffer+0x16)<<16);
    gpu_write_gp0(psx_mod_read_half(buffer+0x18)|(uint32_t)psx_mod_read_half(buffer+0x1A)<<16);
    guest(cpu,0x80068B20,buffer+0x70,16);
    guest(cpu,0x800272B4,0,0);
    if(failed || projected || busy || render_call || hud_call)return 0;
    guest(cpu,0x8006A314,buffer+0xAC,0);
    return !failed;
}
static int local_background(CPUState *cpu,uint32_t address) {
    if(local_background_call || !split_views || !psx_mod_local_view_scope())return 0;
    unsigned layer=cpu->gpr[4];if(layer>=3)return 0;
    local_background_call=1;
    uint32_t start=psx_mod_read_word(SCRATCH+0x108);
    uint32_t result=guest(cpu,address,layer,cpu->gpr[5]);
    uint32_t record=CAMERA+layer*0x54u;
    int sx=(int16_t)psx_mod_read_half(record+10),sy=(int16_t)psx_mod_read_half(record+14);
    int relative=(int8_t)psx_mod_read_byte(record+0x52);
    if(relative>=0 && relative<3) {
        sx=(int16_t)(sx+(int16_t)psx_mod_read_half(CAMERA+(unsigned)relative*0x54u+10));
        sy=(int16_t)(sy+(int16_t)psx_mod_read_half(CAMERA+(unsigned)relative*0x54u+14));
    }
    /* Original 27CD8/281C0: six groups, seventeen page buckets,
     * heads93F78 / tails1D60, 198-byte parity stride. Resolve the complete
     * authored map for a distant view rather than reuse the P1 tile ring. */
    static const PSXCapcomBackground background={0x80093F78u,0x80091D60u,3,17,0x7900};
    unsigned width=psx_mod_read_byte(0x800D1DBCu),stride=psx_mod_read_half(0x80091D58u);
    PSXCapcomTileMap map={psx_mod_read_word(SCRATCH+4),psx_mod_read_word(SCRATCH+8),
        psx_mod_read_word(SCRATCH+12),width,width?stride/width:0,stride,layer,
        psx_mod_read_byte(record+0x4D),psx_mod_read_byte(record+0x4E)};
    WsViewAnchor view={0,16,0,0,0};
    int packets=psx_capcom_background_render(&background,view_background_arena,psx_mod_read_word(SCRATCH),
        &map,sx,sy,view,start,NULL);
    if(packets<0) {failed=1;psx_mod_counter_add("mmx5.coop.local-view-background-invalid",1);}
    else psx_mod_counter_add("mmx5.coop.local-view-background-tiles",(unsigned)packets);
    local_background_call=0;cpu->gpr[2]=result;return 1;
}
static int present_local_view(CPUState *cpu,uint32_t address) {
    if(!split_available() || local_view_call || projected || busy || scene_owner ||
       psx_mod_read_byte(PLAY+1) || psx_mod_read_byte(PLAY+0xF) || psx_mod_read_byte(PLAY+0x10) ||
       psx_mod_read_byte(PLAY+0x1C) || cpu->gpr[31]!=0x80011E50)return 0;
    uint32_t buffer=psx_mod_read_word(0x8009E794u);
    if(cpu->gpr[4]!=buffer+0xAC)return 0;
    local_view_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    guest(cpu,0x80068634,0,0);
    int port=psx_netplay_local_port();local_view_seat=split_view_seat(port>=0 && port<2?(unsigned)port:0);
    PSXModRenderPass rectangle={sizeof rectangle,0,psx_mod_read_half(buffer+0x14),psx_mod_read_half(buffer+0x16),
        psx_mod_read_half(buffer+0x18),psx_mod_read_half(buffer+0x1A)};
    LocalViewContext context={first,second,{0},{0},{0},{0},world_packets,primary_packets,
        projected,busy,world_owner,render_call,render_primary,hud_call,hud_seat,hud_fade,upload_call,failed};
    memcpy(context.play,saved_play,sizeof saved_play);memcpy(context.camera,saved_camera,sizeof saved_camera);
    memcpy(context.car_owners,car_source_owner,sizeof car_source_owner);memcpy(context.resources,resources,sizeof resources);
    uint32_t before=state_digest();
    int committed=psx_mod_render_local_view(cpu,&rectangle,draw_local_view,&buffer);
    local_background_call=0;
    first=context.first;second=context.second;
    memcpy(saved_play,context.play,sizeof saved_play);memcpy(saved_camera,context.camera,sizeof saved_camera);
    memcpy(car_source_owner,context.car_owners,sizeof car_source_owner);memcpy(resources,context.resources,sizeof resources);
    world_packets=context.world_packets;primary_packets=context.primary_packets;
    projected=context.projected;busy=context.busy;world_owner=context.world_owner;
    render_call=context.render_call;render_primary=context.render_primary;hud_call=context.hud_call;
    hud_seat=context.hud_seat;hud_fade=context.hud_fade;upload_call=context.upload_call;failed=context.failed;
    if(state_digest()!=before)psx_mod_counter_add("mmx5.coop.local-view-state-leak",1);
    psx_mod_counter_add(committed?"mmx5.coop.local-view-committed":"mmx5.coop.local-view-refused",1);
    if(committed)psx_mod_counter_add(local_view_seat?"mmx5.coop.local-view-seat-1":"mmx5.coop.local-view-seat-0",1);
    local_view_call=0;cpu->gpr[2]=result;return 1;
}
static void activate(void) {
    char option[16];keep_zero=!psx_mod_netplay_is_active() &&
        psx_mod_current_option_value("keep_zero",option,sizeof option) && !strcmp(option,"true");
    if(!psx_mod_current_option_value("cameras",option,sizeof option))strcpy(option,"unified");
    split_views=(unsigned)mmx5_coop_views_requested(psx_mod_netplay_is_active(),option);
    split_valid=split_owner=split_camera_call=split_scan_call=split_supplemental=split_bounds_call=0;
    local_view_call=local_view_seat=local_background_call=0;split_table=0;
    ai_pool_call=ai_call=ai_owner=ai_suspended=0;ai_pool=ai_pending=ai_function=0;ai_first_freeze=0;
    spatial_call=0;
    memset(split_cameras,0,sizeof split_cameras);memset(split_visits,0,sizeof split_visits);
    for(unsigned i=0;i<view_bank_count;++i) {free(view_banks[i].pixels);view_banks[i].pixels=NULL;}
    view_bank_count=view_ui_bank_count=0;
    view_background_arena=split_views?psx_capcom_background_activate(3):0;
    for(unsigned i=0;i<bank_count;++i) {free(banks[i].pixels);banks[i].pixels=NULL;}
    bank_count=ui_bank_count=0;
    busy=projected=enrolled=failed=ready=ticks=world_ticks=camera_call=0;previous_input=0;
    stage_call=player_call=world_owner=render_call=0;mmx5_coop_lifecycle_init(&life);
    render_primary=0;asset_generation=1;primary_packets=0;
    hud_call=hud_seat=hud_icon_call=0;hud_fade=128;
    pause_owner=-1;p2_start_previous=0;upload_call=menu_active=0;
    for(unsigned i=0;i<menu_backup_count;++i) {free(menu_backups[i].pixels);menu_backups[i].pixels=NULL;}
    menu_backup_count=0;
    camera_update_call=0;
    ever_enrolled=last_stage=section_pending=stage_departed=0;
    hit_count=allocation_call=solid_call=contact_call=0;
    memset(hit_history,0,sizeof hit_history);memset(solid_history,0,sizeof solid_history);
    ride_call=0;
    memset(car_source_owner,0,sizeof car_source_owner);
    for(unsigned i=0;i<2;++i) {
        car_mirror[i]=psx_mod_alloc_guest_memory(0xB0,16);if(!car_mirror[i])failed=1;
    }
    pickup_call=0;memset(pickup_owners,0,sizeof pickup_owners);
    scene_owner=scene_phase=scene_call=door_call=0;scene_y=0;memset(scene_body,0,sizeof scene_body);
    join_phase=0;join_y=0;memset(returning_body,0,sizeof returning_body);
    effect_allocate_call=effect_init_call=0;memset(death_origins,0,sizeof death_origins);fixture_sequence=0;
    memset(&first,0,sizeof first);memset(&second,0,sizeof second);
    if(split_views && !view_background_arena)failed=1;
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
    diagnostic=psx_mod_alloc_guest_memory(0xC00,16);if(!arena || !primary_arena || !hud_arena || !diagnostic)failed=1;
    psx_mod_set_rewind_blocked(1);psx_mod_set_savestate_blocked(1);
    psx_mod_counter_add("mmx5.coop.activated",1);
    psx_mod_counter_add("mmx5.coop.diagnostic-address",diagnostic);
    if(split_views)psx_mod_counter_add("mmx5.coop.split-state",diagnostic+0x700);
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
        split_views,split_valid,split_owner,split_table,
        ai_pool_call,ai_call,ai_owner,ai_suspended,ai_pool,ai_pending,ai_function,ai_first_freeze,
        ever_enrolled,last_stage,section_pending,stage_departed,previous_input,
        life.status[0],life.status[1],life.select_frames,life.select_armed,life.select_previous,life.wipe,
        join_phase,(uint32_t)join_y,(unsigned)(pause_owner+1),(unsigned)p2_start_previous,menu_active,
        scene_owner,scene_phase,(uint32_t)scene_y};
    for(unsigned i=0;i<sizeof words/sizeof words[0];++i)hash=hash_word(hash,words[i]);
    hash=hash_bytes(hash,second.body,sizeof second.body);
    hash=hash_bytes(hash,second.shots,sizeof second.shots);
    hash=hash_bytes(hash,second.trails,sizeof second.trails);
    hash=hash_bytes(hash,first.car,sizeof first.car);hash=hash_bytes(hash,second.car,sizeof second.car);
    hash=hash_bytes(hash,car_source_owner,sizeof car_source_owner);
    hash=hash_bytes(hash,returning_body,sizeof returning_body);
    hash=hash_bytes(hash,scene_body,sizeof scene_body);
    hash=hash_bytes(hash,split_cameras[0],sizeof split_cameras);hash=hash_bytes(hash,split_visits,sizeof split_visits);
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
    for(unsigned i=0;i<64;++i) {
        hash=hash_word(hash,solid_history[i].sides);hash=hash_word(hash,solid_history[i].carried);
        hash=hash_word(hash,solid_history[i].car_sides);hash=hash_word(hash,solid_history[i].car_carried);
    }
    hash=hash_word(hash,ride_call);
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
        NULL,state_digest,1,ID,"coop",NULL,NULL};
    psx_mod_register_netplay_profile(&profile);
    psx_mod_register_activation_plugin(ID,activate);
    psx_mod_register_vblank_plugin(ID,observe_stage);
    psx_mod_register_function_entry_plugin(ID,0x8003BAC8,reset);
    psx_mod_register_function_filter_plugin(ID,0x80022084,attacks_tick);
    psx_mod_register_function_entry_plugin(ID,0x80022D98,world_tick);
    psx_mod_register_function_filter_plugin(ID,0x80024C60,render_hook);
    psx_mod_register_function_filter_plugin(ID,0x80031DD0,contact);
    psx_mod_register_function_filter_plugin(ID,0x800317B8,contact);
    psx_mod_register_function_filter_plugin(ID,0x8002A720,camera_target);
    psx_mod_register_function_filter_plugin(ID,0x8002A7E0,camera_target);
    psx_mod_register_function_filter_plugin(ID,0x8002A440,camera_update);
    psx_mod_register_function_filter_plugin(ID,0x8002A554,split_camera_finish);
    psx_mod_register_function_filter_plugin(ID,0x8002AE50,split_scan);
    psx_mod_register_function_filter_plugin(ID,0x8002B4E4,split_allocate);
    psx_mod_register_instruction_plugin(ID,0x8002B39C,0xA2420000,split_spawned);
    psx_mod_register_function_filter_plugin(ID,0x8002D800,split_actor_bounds);
    psx_mod_register_function_filter_plugin(ID,0x8002D938,split_actor_bounds);
    psx_mod_register_function_filter_plugin(ID,0x8002D89C,split_actor_bounds);
    psx_mod_register_function_filter_plugin(ID,0x8002D9EC,split_actor_bounds);
    psx_mod_register_function_filter_plugin(ID,0x8002DAA0,split_actor_bounds);
    psx_mod_register_function_filter_plugin(ID,0x80068C28,present_local_view);
    psx_mod_register_function_filter_plugin(ID,0x80027F88,local_background);
    psx_mod_register_function_filter_plugin(ID,0x80021F64,ai_pool_tick);
    psx_mod_register_function_filter_plugin(ID,0x800221B0,ai_pool_tick);
    psx_mod_register_function_filter_plugin(ID,0x8005609C,spatial_stage_actor);
    psx_mod_register_function_filter_plugin(ID,0x80056AA0,spatial_stage_actor);
    psx_mod_register_instruction_plugin(ID,0x80022044,0x00A0F809,ai_redirect);
    psx_mod_register_instruction_plugin(ID,0x80022290,0x00A0F809,ai_redirect);
    psx_mod_register_guest_function_plugin(ID,AI_DISPATCH,ai_update);
    psx_mod_register_function_filter_plugin(ID,0x80035040,player_tick);
    psx_mod_register_function_filter_plugin(ID,0x80022960,ride_tick);
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
