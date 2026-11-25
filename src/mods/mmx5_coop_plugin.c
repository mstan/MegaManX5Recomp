/* SLUS-01334 native co-op development implementation.
 * Original 80022D98 runs the shared world once. The hook at its attack-pool
 * boundary adds only the counterpart's input, body, attacks and terrain pass.
 */
#include "mod_plugins.h"
#include "mod_snapshot.h"
#include "mod_netplay.h"
#include "mmx5_coop_host_profile.h"
#include "mod_capcom_bg2d.h"
#include "psx_netplay.h"
#include "cpu_state.h"
#include "sio.h"
#include "gpu.h"
#include "gpu_render.h"
#include "mmx5_coop_assets.h"
#include "mmx5_resident_data.h"
#include "mmx5_coop_memory.h"
#include "mmx5_coop_sprite_packet.h"
#include "mmx5_coop_texture_cache.h"
#include "mmx5_coop_lifecycle.h"
#include "mmx5_coop_scene_anchor.h"
#include "mmx5_coop_views.h"
#include "mmx5_coop_internal.h"
#include "mmx5_coop_snapshot.h"
#include "mmx5_coop_forms.h"
#include "mmx5_coop_armor.h"
#include "mmx5_coop_bindings.h"
#include "mmx5_coop_duel_damage.h"
#include "mmx5_coop_duel_appearance.h"
#include "mmx5_diagnostics.h"
#include "mmx5_widescreen_internal.h"
#include "host_osd.h"
#include <stdlib.h>
#include <string.h>

#define ID "mmx5.coop"
#define PLAYER 0x8009A0A0u
#define PLAY 0x800D1C00u
#define PAD 0x800C931Cu
#define SHOTS 0x80098120u
#define GLOW 0x800942A8u
#define TRAILS 0x8009A2F8u
#define CAMERA 0x8009A1F8u
#define CAR 0x800D1DC0u
#define SCRATCH 0x1F800000u
#define FRAME_BYTES 0x40000u
/* Native meter/icon blocks end at Y116. Stack P2 below that block while
 * retaining the original life/ammo geometry, badges, colors and animation. */
#define SECOND_HUD_Y 104
#define DUEL_HP 48u

typedef struct {
    uint8_t body[0x158], shots[0x1380], glow[0xF0], trails[0x120], car[0xB0];
} PlayerContext;
typedef struct {
    uint8_t *graphics;
    size_t graphics_bytes;
    uint8_t *messages;
    size_t message_bytes;
    uint32_t message_root;
    uint32_t sprites, assembly, colors, palette, menu_colors, menu_layout;
    uint16_t *ui_pixels;
    uint16_t hud_colors[16];
    unsigned loaded,record,identity;
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
/* Immutable native sources share the existing two guest ranges. UI bank IDs
 * retain their original source even after a quiescent X body reinitializes. */
static CharacterAssets source_assets[MMX5_COOP_ARMOR_SOURCES];
static unsigned selected_x_armor=1,armor_selection_seen,armor_ui_call;
static unsigned selected_zero_form,armor_selecting_seat,armor_list_seat;
static uint32_t armor_empty_parts,armor_name_frame;
static unsigned armor_name_call;
static uint16_t armor_previous_input[2];
static uint8_t armor_native_roster[6];
static unsigned armor_native_count,armor_roster_active;
/* Immutable host graphics plus a guest assembly directory. Native24C60 and
 *26CAC read assembly/pose, never compressed graphics. The draw decoder uses
 *the existing X range as source identity with DAT93's separate record tag;
 *it never overwrites the equipped X guest resources. Expansion1 is only1MiB. */
static CharacterAssets duel_x_art;
static CharacterAssets *duel_render_art;
static unsigned x_asset_source;
static uint32_t hash_bytes(uint32_t hash,const uint8_t *data,size_t size);
static PlayerContext first, second;
static unsigned separate_etanks,etanks_initialized;
static uint8_t private_tank_contents[2][3];
static uint8_t saved_play[0x1C0], saved_camera[0xFC];
static uint32_t resources[6];
static const unsigned resource_offsets[6]={0x14,0x1C,0x24,0x28,0x38,0x3C};
static SpriteBank banks[4096];
static unsigned bank_count, counterpart, projected, busy, enrolled, failed, ready, camera_call;
static SpriteBank view_banks[4096];
static unsigned view_bank_count;
static unsigned split_views,split_valid,split_owner,split_camera_call,split_scan_call,split_supplemental;
static unsigned split_bounds_call,local_view_call,local_view_seat,local_hud_seat;
static unsigned local_split,local_split_wide,local_aspect_set;
static unsigned spatial_call;
static unsigned personal_effect_call;
static unsigned personal_visual_call;
static uint32_t personal_visual_ticks[2];
static uint8_t personal_effect_owner[96];
static unsigned area_call;
static uint32_t area_actor,area_function;
#define AREA_DISPATCH 0x8F7FFF04u
static uint8_t split_cameras[2][0xFC],split_visits[MMX5_COOP_PLACEMENTS/8];
static uint32_t split_table;
static unsigned ai_pool_call,ai_call,ai_owner,ai_suspended;
/* A borrowed AI view may issue a quake for the canonical view too. This
 * pointer exists only inside ai_update's synchronous guest call; it is not
 * simulation state and never survives that call or a snapshot boundary. */
static uint8_t *ai_camera_restore;
static uint32_t ai_pool,ai_pending,ai_function;
static uint8_t ai_first_freeze;
/* The lower fire and upper chaser occupy mutually exclusive stage overlays.
 * Both retain one native sequence camera through admitting-hero death and
 * spectator-camera reassignment; their serialized storage is shared. */
static struct {uint32_t actor;uint8_t owner,camera[0xFC];} dinorex_sequence;
/* BIN9's stair atlas has one native frame/remainder for each moving view. */
static struct {uint32_t actor;uint8_t pose[4],valid;} glow_stair;
#define AI_DISPATCH 0x8F7FFF00u
static unsigned stage_call, player_call, world_owner, render_call;
static unsigned camera_update_call;
static void rose_private_camera_area(void);
static void dinorex_private_camera_focus(void);
static uint32_t world_packets;
static Mmx5CoopLifecycle life;
typedef struct {uint32_t x,y,assembly;uint8_t layer,valid;} DeathOrigin;
static DeathOrigin death_origins[128];
/* Native43390's Zero landing effect retains its resource directories.
 * Native stage effects use only the same serialized logical owner field.
 * DeathOrigin remains the separate one-shot death-position contract. */
typedef struct {uint32_t sprites,assembly;uint8_t owner;} LandingEffect;
static LandingEffect landing_effects[128];
static unsigned effect_allocate_call,effect_init_call;
static uint32_t fixture_sequence;
static uint32_t diagnostic;
#ifndef PSX_NO_DEBUG_TOOLS
static unsigned fixture_location_pending,fixture_stage,fixture_section,fixture_checkpoint;
static unsigned fixture_scenery_pending;
static uint32_t fixture_damage_actor;
static unsigned fixture_damage_stage,fixture_damage_kind;
/* Original8002ACC4 indexes these resident arrays. Validate the complete
 * pointer chain before admitting an offline location request. */
static int fixture_location_valid(unsigned stage,unsigned section,unsigned checkpoint) {
    if(stage>22 || section>1 || checkpoint>127)return 0;
    uint32_t table=psx_mod_read_word(0x80072D10u+(stage*2u+section)*4u);
    if(table<0x80072B4Cu || table>=0x80072D10u || (table&3u))return 0;
    uint32_t end=0x80072D10u;
    for(unsigned i=0;i<46;++i) {
        uint32_t next=psx_mod_read_word(0x80072D10u+i*4u);
        if(next>table && next<end)end=next;
    }
    if(checkpoint>=(end-table)/4u)return 0;
    uint32_t record=psx_mod_read_word(table+checkpoint*4u);
    return record>=0x80071B68u && record+0x24u<=0x80072B4Cu &&
        !((record-0x80071B68u)%0x24u);
}
static void fixture_mission_begin(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(!fixture_location_pending || psx_mod_netplay_is_active() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=5 || !fixture_location_valid(fixture_stage,fixture_section,fixture_checkpoint))return;
    /* Finish the old world's actor/graphics passes before changing its stage
     * identity. Native20B80 then owns the new mission title and disc loader;
     * native2ACC4 still consumes only the requested checkpoint record. */
    psx_mod_write_byte(PLAY+0x0C,(uint8_t)fixture_stage);
    psx_mod_write_byte(PLAY+0x0D,(uint8_t)fixture_section);
    psx_mod_write_byte(PLAY+0x1D,0);psx_mod_write_byte(PLAY+0x1E,0);
    psx_mod_counter_add("mmx5.coop.fixture-missions-applied",1);
}
static void fixture_checkpoint_begin(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(!fixture_location_pending || psx_mod_netplay_is_active() || psx_mod_local_view_scope())return;
    /* Mission-title overlays also initialize a camera. Keep the request until
     * the actual native mode9 stage initializer (call800204C4) consumes it. */
    if(psx_mod_read_byte(PLAY)!=9)return;
    if(psx_mod_read_byte(PLAY+0x0C)==fixture_stage &&
       psx_mod_read_byte(PLAY+0x0D)==fixture_section &&
       fixture_location_valid(fixture_stage,fixture_section,fixture_checkpoint)) {
        psx_mod_write_byte(PLAY+0x1D,(uint8_t)fixture_checkpoint);
        fixture_scenery_pending=fixture_stage==3 && !fixture_section &&
            (fixture_checkpoint==4 || fixture_checkpoint==5);
        fixture_scenery_pending|=fixture_stage==8 && !fixture_section &&
            fixture_checkpoint>=2 && fixture_checkpoint<=4;
        if(diagnostic) {
            psx_mod_write_word(diagnostic+0x9C,fixture_stage|(fixture_section<<8)|(fixture_checkpoint<<16));
            uint32_t table=psx_mod_read_word(0x80072D10u+(fixture_stage*2u+fixture_section)*4u);
            psx_mod_write_word(diagnostic+0xA0,psx_mod_read_word(table+fixture_checkpoint*4u));
        }
        psx_mod_counter_add("mmx5.coop.fixture-checkpoints-applied",1);
    }else psx_mod_counter_add("mmx5.coop.fixture-checkpoints-rejected",1);
    fixture_location_pending=0;
}
static void fixture_scenery_begin(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(!fixture_scenery_pending || psx_mod_netplay_is_active() ||
       psx_mod_local_view_scope() || psx_mod_read_byte(PLAY)!=10)return;
    fixture_scenery_pending=0;
    /* BIN4's EFDCC requests the lower sheet before leaving the preceding
     * whale. CP4/5 shortcuts skip that prerequisite. Queue the same native
     * request after stage initialization (276C4 clears an earlier request),
     * before this first gameplay update's 289D8/LoadImage. This is debug
     * setup only, not whale defeat or drop/transition completion. */
    int whale=psx_mod_read_byte(PLAY+12)==3 && !psx_mod_read_byte(PLAY+13) &&
       (psx_mod_read_byte(PLAY+0x1D)==4 || psx_mod_read_byte(PLAY+0x1D)==5) &&
       psx_mod_read_word(0x800EE970u)==8u &&
       psx_mod_read_word(0x800EFDCCu)==0xAC5192F8u;
    /* Pegasus CP2..4 shortcuts skip original door06 at2320/2080. Its
     * native54D9C completion requests DAT111's replacement sheet; later
     * door0B does not repeat that request. Restore this debug prerequisite
     * after stage init, never add an upload to the production door. */
    unsigned checkpoint=psx_mod_read_byte(PLAY+0x1D);
    int pegasus=psx_mod_read_byte(PLAY+12)==8 && !psx_mod_read_byte(PLAY+13) &&
       checkpoint>=2 && checkpoint<=4 && psx_mod_read_word(0x800EE970u)==15u &&
       psx_mod_read_word(0x80054E50u)==0xAC5192F8u &&
       psx_mod_read_word(0x800F8C64u)==0x04060800u &&
       psx_mod_read_word(0x800F8C68u)==0x08200910u;
    if((whale || pegasus) && psx_mod_read_word(0x80028A0Cu)==0xAE0392F8u &&
       !psx_mod_read_word(0x800E92F8u)) {
        psx_mod_write_word(0x800E92F8u,1);
        psx_mod_counter_add("mmx5.coop.fixture-scenery-prerequisites",1);
    }
}
#endif
#ifdef MMX5_COOP_TEST_CHEATS
static unsigned test_chord,test_god_mask,test_contact_mask,test_commands;
static void test_menu_commands(void);
#endif
static uint32_t arena, primary_arena, primary_packets, ticks, world_ticks, asset_generation;
static unsigned second_terrain_pending;
static unsigned render_primary;
static uint16_t previous_input;
static uint8_t returning_body[0x158];
static int32_t join_y;
static unsigned join_phase;
typedef struct {uint16_t page,clut,uv,id;uint16_t colors[16];unsigned source,reflection;} UiBank;
static UiBank ui_banks[4096];
static unsigned ui_bank_count,hud_call,hud_seat,hud_icon_call,hud_fade=128;
static unsigned hud_side_by_side;
static UiBank view_ui_banks[4096];
static unsigned view_ui_bank_count;
static uint32_t hud_arena;
static uint16_t retained_tile_bank(uint16_t page,uint16_t clut,uint16_t uv,int personal,unsigned reflection,unsigned directory,unsigned character);
typedef struct {unsigned x,y,w,h;uint16_t *pixels;} MenuBackup;
static MenuBackup menu_backups[32];
static unsigned menu_backup_count,menu_active,upload_call;
static int pause_owner=-1,p2_start_previous;
/* Later X4 personal-menu feature; native MENU and original weapon are per seat. */
typedef struct {uint8_t native[0x3C],original_weapon;} PersonalMenu;
static PersonalMenu personal_menus[2];
static uint16_t personal_menu_previous[2];
static unsigned personal_active,personal_initialized,personal_eligible;
static unsigned personal_menu_call,personal_menu_seat,personal_rendering,personal_shared_call;
static void personal_menu_reset(void);
static void menu_vram(CPUState *cpu,int active);
static unsigned ever_enrolled,last_stage,last_section,section_pending,stage_departed;
typedef struct {uint32_t actor;uint8_t token[2],capture_owner;} HitHistory;
static HitHistory hit_history[256];
static unsigned hit_count,allocation_call,solid_call,contact_call;
typedef struct {uint8_t sides,carried,car_sides,car_carried,controller_p2;} SolidHistory;
static SolidHistory solid_history[64];
typedef struct {uint32_t actor;uint8_t seat;} PickupOwner;
static PickupOwner pickup_owners[128];
static unsigned pickup_call;
static unsigned scene_owner,scene_phase,scene_call,door_call,paired_victory,scene_corridor_arrow,scene_capsule;
typedef struct {
    uint32_t actor;
    int16_t old_min,old_max,target_min,target_max,right;
    uint8_t stage,section,phase,owner,owns_lock;
} BothDoor;
static BothDoor both_door;
/* The non-entering seat keeps its ordinary body, attacks and menu. The native
 * opponent supplies the authored entrance/position and right-meter shell. */
typedef struct {
    uint32_t door,opponent,x,y,box,damage_table;
    uint8_t phase,entrant,character[2],facing[2],token[2],winner,native_body[0x9C],colors[64];
    uint8_t intro_complete,arrived,health[2];
} StoryDuel;
static StoryDuel story_duel;
/* Zero means no result;1=X,2=Zero. Story selection never changes either
 * physical body, normal menu identity or native starting-character rights. */
static uint8_t duel_story_character;
static unsigned duel_opponent_call,duel_text_call;
/* Synchronous native contact transactions never survive a frame boundary. */
static unsigned duel_contact_call,duel_native_contact,duel_attacker,duel_transfer_failed,duel_native_injury;
static unsigned both_door_players,door_wait,door_traversal_call,door_terrain_call;
static uint8_t scene_body[0x158];
static int32_t scene_y;
static unsigned keep_zero;
static unsigned ride_call,ride_spawn_call,ride_deferred_slots;
static uint32_t extra_ride_record;
static int native_armor_overlay(void);
static uint32_t car_mirror[2];
static uint8_t car_source_owner[272];
static int chaser_body_active(const uint8_t *body) {
    return body[4]==1 && body[0xD5]==0xFF;
}
static int chaser_exit_active(const uint8_t *car,unsigned seat) {
    /* D5 clears at dismount, before BIN7's phase2 animation, Alia/orb
     * program and native3D2C4 release. Retain/tick that vehicle context
     * until its original retirement; otherwise capture_player erases the
     * script and leaves the counterpart held on foot indefinitely. */
    return enrolled && life.status[seat]==MMX5_COOP_ALIVE &&
        car[0] && !car[1] && car[4]==2;
}
static int car_context(void) {
    /* A dead rider retains D5=FF after the survivor's native dismount.
     * It must not keep on-foot dialogue/capsules in the vehicle context. */
    return psx_mod_read_byte(PLAY+0x0C)==5 && psx_mod_read_word(0x800EE970)==11 &&
        ((!enrolled && !psx_mod_read_byte(PLAY+0x1D)) ||
         (psx_mod_read_byte(PLAYER+4)==1 && psx_mod_read_byte(PLAYER+0xD5)==0xFF) ||
         chaser_body_active(first.body) || chaser_body_active(second.body) ||
         chaser_exit_active(first.car,0) || chaser_exit_active(second.car,1) ||
         (psx_mod_read_byte(CAR) && !psx_mod_read_byte(CAR+1) &&
          (psx_mod_read_byte(CAR+4)==1 ||
           (psx_mod_read_byte(CAR+4)==2 && psx_mod_read_byte(PLAYER+4)==1)) &&
          (psx_mod_read_byte(CAR+0x5C)&127u)));
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
static void scene_recover_owner(CPUState *cpu);
static void split_scan_rectangle(CPUState *cpu,PSXPlacementRect region);
static void begin_join(CPUState *cpu,uint32_t x,uint32_t y);
static int standing_return_position(CPUState *cpu,uint32_t *x,uint32_t *y,
        int spaced,int support);
static uint32_t state_digest(void);
static void balance_reset(void);
static void balance_forget(uint32_t actor);
static int snapshot_resolve_bank(uint16_t id);
static void scene_begin(unsigned seat);
static int grizzly_stage(void);
static int corridor_arrow_near(int x,int y);
static void intro_private_layer_region(void);
static void rose_private_layer_region(void);
static int rose_stage_program(void);
static int intro_boss_text_skip(CPUState *cpu,uint32_t address);
static void both_door_maintain(CPUState *cpu);
static int both_living(unsigned seat);
static uint8_t both_body_byte(unsigned seat,unsigned at);
static uint32_t both_body_word(unsigned seat,unsigned at);
static void both_body_word_write(unsigned seat,unsigned at,uint32_t value);
static int split_available(void);
static void put32(uint8_t *p,uint32_t value);
static int duel_scope(void);
static int duel_loser_body(void);
static int duel_inner_door(uint32_t actor);
static void duel_begin(CPUState *cpu,uint32_t actor,unsigned seat);
static int duel_hold_opponent(CPUState *cpu,uint32_t actor);
static int duel_return_anchor(Mmx5CoopReturnAnchor *anchor);
static void duel_after_world(CPUState *cpu);
static int duel_hud_active(void);
static int duel_scope(void);
static int both_door_scope(void) {
    return both_door.phase && both_door.stage==psx_mod_read_byte(PLAY+0x0C) &&
        both_door.section==psx_mod_read_byte(PLAY+0x0D);
}
static int both_door_active(void) {return both_door_scope() && both_door.phase<4;}
static int both_door_native_carry(void) {
    /* Both changes admission, not the original door's traversal. Reuse its
     * one scripted carry for ordinary doors in every stage, including their
     * camera/layer commands and native closing test. The companion receives
     * the same displacement through both_door_traversal, without another
     * world tick or a free-movement interval inside the aperture. */
    return both_door_scope() && psx_mod_read_byte(both_door.actor+1)==8;
}
static int scene_passenger(unsigned seat) {return scene_owner && !paired_victory && scene_owner!=seat+1;}
static void clear_personal_drawables(unsigned seat);
static void enforce_roster(void) {
    if(!enrolled || !counterpart || mmx5_coop_zero_available(
        psx_mod_read_byte(PLAY+0x79),psx_mod_read_byte(PLAY+0xCB),keep_zero))return;
    life.status[1]=MMX5_COOP_UNAVAILABLE;second.body[3]=0;second.body[4]=3;
    memset(second.shots,0,sizeof second.shots);clear_personal_drawables(1);
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
    capture(GLOW,p->glow,sizeof p->glow);
    capture(TRAILS,p->trails,sizeof p->trails);
    if(car_context())capture(CAR,p->car,sizeof p->car);
    else memset(p->car,0,sizeof p->car);
}
static void project_player(const PlayerContext *p) {
    project(PLAYER,p->body,sizeof p->body);
    project(SHOTS,p->shots,sizeof p->shots);
    project(GLOW,p->glow,sizeof p->glow);
    project(TRAILS,p->trails,sizeof p->trails);
    if(car_context())project(CAR,p->car,sizeof p->car);
}
static int resident_personal_giga(void) {
    return psx_mod_read_byte(PLAYER) && !psx_mod_read_byte(PLAYER+2) &&
        psx_mod_read_byte(PLAYER+4)==1 &&
        ((psx_mod_read_byte(PLAYER+5)==0x22 && psx_mod_read_byte(PLAYER+0xFA)==4) ||
         (psx_mod_read_byte(PLAYER+5)==0x24 && psx_mod_read_byte(PLAYER+0xFA)==2));
}
static int personal_giga(void) {
    if(!enrolled || failed || scene_owner)return 0;
    if(resident_personal_giga())return 1;
    const uint8_t *body=projected?first.body:second.body;
    /* Native3E390/3E348 enter Ultimate22/Falcon24 and assert PLAY+1C. The
     * attack belongs to that body; it is not a shared room transition.
     * Gaea25 remains outside this qualified personal attack path. */
    return body[0] && !body[2] && body[4]==1 &&
        ((body[5]==0x22 && body[0xFA]==4) || (body[5]==0x24 && body[0xFA]==2));
}
static void partner_giga_control(void) {
    /* Only call inside a private projection. leave_second restores the
     * attack owner's canonical lock, including its native recovery. */
    if(personal_giga() && !resident_personal_giga())psx_mod_write_byte(PLAY+0x1C,0);
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
        unsigned following=psx_mod_read_word(actor+0x50)==CAR;
        /* Original4D4F4 records Chaser debris' parent at +54. Its deferred
         * 4D818 init copies the parent's assembly/CLUT after the private
         * car has been restored, so retain that same seat's mirror too. */
        unsigned debris=i>=144 && psx_mod_read_byte(actor+1)==3 &&
            !psx_mod_read_byte(actor+2) && psx_mod_read_word(actor+0x54)==CAR;
        if(!following && !debris)continue;
        if(!car_source_owner[i])car_source_owner[i]=(uint8_t)(default_seat+1);
        uint32_t source=car_mirror[car_source_owner[i]-1];
        if(following)psx_mod_write_word(actor+0x50,source);
        if(debris)psx_mod_write_word(actor+0x54,source);
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
    /* Original contact checks any nonzero A4; 38C54..38C7C clears it when
     * timer61 ends. Positive A4 also suppresses falling in 396E8 and aerial
     * movement in 399D4. Use the native negative blinking state so a healthy
     * return remains protected without walking beyond its landing floor. */
    psx_mod_write_byte(PLAYER+0x61,60);psx_mod_write_byte(PLAYER+0xA4,255);
    guest(cpu,0x8003C5D8,PLAYER,0);
}
static uint32_t foot_aligned_y(uint32_t y,uint32_t leader_box,uint32_t partner_box) {
    if(leader_box<0x80010000u || leader_box>0x801FFFFCu ||
       partner_box<0x80010000u || partner_box>0x801FFFFCu)return y;
    /* Native boxes contain signed center offsets and unsigned half extents.
     * X's standing bottom is +21, Zero's +22. Sharing center Y leaves X a
     * pixel above a Zero-owned floor, where a stair can block walking. */
    int leader_foot=(int8_t)psx_mod_read_byte(leader_box+1)+psx_mod_read_byte(leader_box+3);
    int partner_foot=(int8_t)psx_mod_read_byte(partner_box+1)+psx_mod_read_byte(partner_box+3);
    return y+(uint32_t)((leader_foot-partner_foot)*65536);
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
    /* Original SLUS-01334 table80070368. Activation precedes EXE RAM load:
     * immutable UI preparation must not read uninitialized guest memory. */
    static const uint16_t positions[19][2]={{320,256},{960,256},{384,0},{576,0},
        {640,0},{704,0},{320,176},{832,256},{896,256},{448,0},{512,0},{320,112},
        {960,160},{576,0},{448,128},{576,128},{448,144},{448,16},{576,16}};
    unsigned x=positions[type][0],y=positions[type][1],base_y=y&256;
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
static int ui_decode_compressed(const CharacterAssets *character,unsigned directory,
                                uint8_t decoded[32768],unsigned *count) {
    /* Native50D04/17348 installs frame0 from a character graphics group at
     * an authored VRAM rectangle. Retain the same unpaletted words alongside
     * static NIM images; never sample P1's later replacement of that region. */
    const uint8_t *gfx=character->graphics;size_t size=character->graphics_bytes;
    if(directory+4u>size)return 0;
    uint32_t table=le32(gfx+directory);
    if(table>size || size-table<4)return 0;
    uint32_t packed=le32(gfx+table),tiles=packed>>20;
    size_t source=(size_t)table+(packed&0xFFFFFu),written;
    if(!tiles || tiles>256 || source>=size ||
       !mmx5_coop_decode_sprite(gfx+source,size-source,decoded,32768,&written) ||
       written!=(size_t)tiles*128)return 0;
    *count=tiles;return 1;
}
static int ui_upload_compressed(CharacterAssets *character,unsigned directory,unsigned x,unsigned y) {
    uint8_t decoded[32768];unsigned tiles;
    if(!ui_decode_compressed(character,directory,decoded,&tiles) ||
       x+64>1024 || y+((tiles+15)/16)*16>512)return 0;
    size_t input=0;
    while(tiles) {
        unsigned count=tiles<16?tiles:16,width=count*4;
        for(unsigned row=0;row<16;++row)for(unsigned col=0;col<width;++col) {
            const uint8_t *p=decoded+input+(row*width+col)*2;
            character->ui_pixels[(y+row)*1024+x+col]=(uint16_t)(p[0]|p[1]<<8);
        }
        input+=count*128;y+=16;tiles-=count;
    }
    return 1;
}
static unsigned character_source(unsigned character) {return character?1:x_asset_source;}
static void bind_character_source(unsigned character,unsigned source) {
    CharacterAssets *a=&assets[character],*native=&source_assets[source];
    a->graphics=native->graphics;a->graphics_bytes=native->graphics_bytes;
    a->messages=native->messages;a->message_bytes=native->message_bytes;
    a->ui_pixels=native->ui_pixels;memcpy(a->hud_colors,native->hud_colors,sizeof a->hud_colors);
    a->loaded=native->loaded;a->record=native->record;a->identity=native->identity;
}
static int load_assets(int initialize_guest) {
    if(source_assets[MMX5_COOP_ARMOR_SOURCES-1].loaded) {
        bind_character_source(0,x_asset_source);bind_character_source(1,1);
        if(!initialize_guest)return 1;
        for(unsigned c=0;c<2;++c) {
            project(assets[c].sprites,assets[c].graphics,0x58000);
            project(assets[c].message_root,assets[c].messages,assets[c].message_bytes);
        }
        return mmx5_coop_audio_load(0,NULL,0) && mmx5_coop_audio_load(1,NULL,0);
    }
    uint32_t bytes;
    uint8_t *owned=NULL;
    const uint8_t *dat=mmx5_resident_dat(&bytes);
    /* The active loader already owns verified effective bytes. Reading the
     * whole48MB archive again here can block the first co-op world boundary.
     * All retained graphics/audio are copied below; never free borrowed data.
     * Disabled/reference loaders keep the original effective-disc fallback. */
    if(!dat)dat=owned=disc_file("ROCK_X5.DAT",&bytes);
    if(!dat)return 0;
    (void)mmx5_widescreen_prepare_region_atlases(dat,bytes);
    psx_mod_counter_add(owned?"mmx5.coop.assets-disc-fallback":"mmx5.coop.assets-resident-reuse",1);
    for(unsigned c=0;c<2;++c)if(!mmx5_coop_audio_prepare(c,dat,bytes,0)) {free(owned);return 0;}
    for(unsigned c=0;c<MMX5_COOP_ARMOR_SOURCES;++c) {
        Mmx5AssetView graphics,assembly,colors,messages;
        unsigned character=c==1,record=mmx5_coop_armor_record(c);
        if(!mmx5_coop_dat_asset(dat,bytes,record,3,&graphics) || graphics.type!=2 ||
           !mmx5_coop_dat_asset(dat,bytes,record,9,&assembly) || assembly.type!=3 ||
           !mmx5_coop_dat_asset(dat,bytes,record,0,&colors) || colors.type!=9 ||
           !mmx5_coop_dat_asset(dat,bytes,record,15,&messages) || messages.type!=0x15 ||
           messages.size<0x400 || messages.size>0x12000 ||
           graphics.size>0x40000 || assembly.size>0xC000 || colors.size>0x4000) {
            free(owned);return 0;
        }
        source_assets[c].graphics=calloc(1,0x58000);
        source_assets[c].messages=malloc(messages.size);
        if(!source_assets[c].graphics || !source_assets[c].messages) {free(owned);return 0;}
        source_assets[c].message_bytes=messages.size;
        memcpy(source_assets[c].messages,messages.data,messages.size);
        source_assets[c].graphics_bytes=graphics.size;
        memcpy(source_assets[c].graphics,graphics.data,graphics.size);
        memcpy(source_assets[c].graphics+0x40000,assembly.data,assembly.size);
        memcpy(source_assets[c].graphics+0x4C000,colors.data,colors.size);
        Mmx5AssetView menu_colors,menu_layout;
        if(!mmx5_coop_dat_asset(dat,bytes,record,7,&menu_colors) || menu_colors.type!=17 || menu_colors.size>0x400 ||
           !mmx5_coop_dat_asset(dat,bytes,record,8,&menu_layout) || menu_layout.type!=18 || menu_layout.size>0x1800) {
            free(owned);return 0;
        }
        memcpy(source_assets[c].graphics+0x54000,menu_colors.data,menu_colors.size);
        memcpy(source_assets[c].graphics+0x54400,menu_layout.data,menu_layout.size);
        /* The native mutable palette starts with DAT1/2's complete type5
         * buffer. Type9 above is an immutable color-program source, not a
         * CLUT layout: its rows4..6 contain character colors/blank data where
         * native dust and smoke expect common colors. Original player
         * programs then update their destinations in this private buffer. */
        Mmx5AssetView ui_palette;
        if(!mmx5_coop_dat_asset(dat,bytes,character+1,0,&ui_palette) || ui_palette.type!=5 || ui_palette.size!=0x4000) {
            free(owned);return 0;
        }
        memcpy(source_assets[c].graphics+0x50000,ui_palette.data,ui_palette.size);
        /* Original-disc UI badge CLUT7841 is at member offset220. */
        for(unsigned i=0;i<16;++i)source_assets[c].hud_colors[i]=(uint16_t)(ui_palette.data[0x220+i*2]|
            ((uint16_t)ui_palette.data[0x221+i*2]<<8));
        source_assets[c].ui_pixels=calloc(1024*512,sizeof(uint16_t));
        if(!source_assets[c].ui_pixels) {free(owned);return 0;}
        /* The native common atlas is installed before character images.
         * Death glyphs use it even though their assembly is character-owned. */
        Mmx5AssetView common_image;
        if(!mmx5_coop_dat_asset(dat,bytes,0,0,&common_image) || common_image.type!=0x10002 ||
           !ui_upload(&source_assets[c],&common_image)) {free(owned);return 0;}
        for(unsigned i=0;i<16;++i) {
            Mmx5AssetView item;
            if(!mmx5_coop_dat_asset(dat,bytes,record,i,&item)) {free(owned);return 0;}
            if((item.type&0x10000) && !ui_upload(&source_assets[c],&item)) {free(owned);return 0;}
        }
        /* Gaea's native3C2DC..3C308 queues type1B/sub8 and9. Original50D04
         * selects graphics directory30 and2C, then17348 uploads them at
         * (320,32)/(320,64). Its shot2D has assembly but no graphics pointer. */
        if(record==95 && (!ui_upload_compressed(&source_assets[c],0x30,320,32) ||
                         !ui_upload_compressed(&source_assets[c],0x2C,320,64))) {
            free(owned);return 0;
        }
        source_assets[c].record=record;
        uint32_t identity=hash_bytes(2166136261u,source_assets[c].graphics,0x58000);
        for(unsigned i=0;i<1024*512;++i) {
            uint16_t pixel=source_assets[c].ui_pixels[i];
            identity=(identity^(pixel&255u))*16777619u;
            identity=(identity^(pixel>>8))*16777619u;
        }
        source_assets[c].identity=hash_bytes(identity,source_assets[c].messages,source_assets[c].message_bytes);
        source_assets[c].loaded=1;
    }
    bind_character_source(0,x_asset_source);bind_character_source(1,1);
    free(owned);return !initialize_guest || load_assets(1);
}
/* Called only before constructing/reinitializing private X, never while a
 * live attack is using an old source. Genuine manual code flags remain real. */
static void prepare_private_x(void) {
    if(!source_assets[0].loaded || !source_assets[2].loaded)return;
    unsigned roster=counterpart?psx_mod_read_byte(PLAY+0x49):selected_x_armor;
    unsigned source=mmx5_coop_armor_source(mmx5_coop_forms_roster(0,roster));
    if(source==x_asset_source)return;
    x_asset_source=source;bind_character_source(0,source);
    project(assets[0].sprites,assets[0].graphics,0x58000);
    project(assets[0].message_root,assets[0].messages,assets[0].message_bytes);
}
static void form_body_init(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!failed && !psx_mod_local_view_scope() &&
       cpu->gpr[4]==PLAYER && psx_mod_read_byte(PLAYER+2)==0) {
        prepare_private_x();
        /* A genuine code/capsule grant can precede this native initializer
         * inside an existing projection. Match its new source and parts row;
         * leave_second still restores the canonical roster byte. */
        if(projected && !counterpart)
            psx_mod_write_byte(PLAY+0x49,(uint8_t)mmx5_coop_forms_roster(0,selected_x_armor));
    }
}
void mmx5_coop_zero_normal_palette(void) {
    if(failed || !source_assets[1].loaded ||
       psx_mod_read_byte(PLAYER+2)!=1)return;
    /* Native3C5D8 installs Black Zero's three afterimage color rows,
     * while its Normal branch only installs the body row. Switching back
     * needs the original Normal rows too. Shared world callbacks can retain
     * the stage palette while projecting P2: keep these personal rows private. */
    uint32_t palette=psx_mod_read_word(SCRATCH+0x14)==assets[1].sprites?
        assets[1].palette:psx_mod_read_word(SCRATCH+0x28);
    if(palette!=assets[1].palette && (palette<0x80010000u || palette>0x801FBFFFu))return;
    project(palette+13u*32u,source_assets[1].graphics+0x50000u+13u*32u,3u*32u);
}
static void etanks_seed(void) {
    if(!separate_etanks || etanks_initialized)return;
    for(unsigned i=0;i<sizeof private_tank_contents[0];++i)
        private_tank_contents[0][i]=private_tank_contents[1][i]=psx_mod_read_byte(PLAY+0x76+i)&0x7Fu;
    etanks_initialized=1;
}
static void etanks_capture(unsigned seat) {
    if(!separate_etanks)return;
    etanks_seed();
    for(unsigned i=0;i<sizeof private_tank_contents[seat];++i)
        private_tank_contents[seat][i]=psx_mod_read_byte(PLAY+0x76+i)&0x7Fu;
}
static void etanks_project(unsigned seat) {
    if(!separate_etanks)return;
    etanks_seed();
    unsigned acquired=psx_mod_read_half(PLAY+0x7E);
    for(unsigned i=0;i<sizeof private_tank_contents[seat];++i) {
        /* Acquisition remains shared. The E slots carry a presence bit for
         * native menu compaction, while the W slot stores plain energy. */
        unsigned present=acquired&(0x1000u<<i);
        if(!present)private_tank_contents[seat][i]=0;
        unsigned contents=private_tank_contents[seat][i];
        if(present && i<2)contents|=0x80u;
        psx_mod_write_byte(PLAY+0x76+i,(uint8_t)contents);
    }
}
static void enter_second(void) {
    etanks_capture(0);
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
    psx_mod_write_byte(PLAY+0x49,(uint8_t)(counterpart?5:mmx5_coop_forms_roster(0,selected_x_armor)));
    etanks_project(1);
}
static void leave_second_context(int shared_world) {
    etanks_capture(1);
    capture_player(&second);cars_refresh(1);project_player(&first);
    if(shared_world) {
        /* Native20794 saves the resident body's HP/ammo/selection before a
         * negative exit enters mode9 with continuation policyFF. When P2
         * owns that world pass the singleton cache belongs to P2, but the
         * next native3BAC8 initializes P1 from it. Keep P2 in second.body
         * and restore P1's cache from its canonical context before loading.
         * Fresh/refill entries and native P1 exits keep their own policy. */
        if(saved_play[0]==10 && psx_mod_read_byte(PLAY)==9 &&
           psx_mod_read_byte(PLAY+0x1E)==0xFF) {
            psx_mod_write_byte(PLAY+0x46,first.body[0x5C]);
            project(PLAY+0x56,first.body+0xA8,32);
            psx_mod_write_byte(PLAY+0x7A,first.body[0x93]);
            psx_mod_write_byte(PLAY+0xC1,first.body[0xFB]);
            psx_mod_counter_add("mmx5.coop.p1-continuation-cache-restored",1);
        }
        psx_mod_write_byte(PLAY+0x44,saved_play[0x44]);psx_mod_write_byte(PLAY+0x49,saved_play[0x49]);
    }
    else {
        /* Native rank/infection counters are shared even when a private body
         * pass records them. Original31A94..31AD4 records a full infection
         * at BE and advances Zero's campaign count at A9. Restoring PLAY
         * without these kept P2's body infected but discarded its progression.
         * Timers and scene flags remain world-owned. */
        uint32_t hits=psx_mod_read_word(PLAY+0xB4),damage=psx_mod_read_word(PLAY+0xB8);
        uint16_t infections=psx_mod_read_half(PLAY+0xBE);
        uint8_t zero_infections=psx_mod_read_byte(PLAY+0xA9);
        project(PLAY,saved_play,sizeof saved_play);
        psx_mod_write_word(PLAY+0xB4,hits);psx_mod_write_word(PLAY+0xB8,damage);
        psx_mod_write_half(PLAY+0xBE,infections);psx_mod_write_byte(PLAY+0xA9,zero_infections);
        project(CAMERA,saved_camera,sizeof saved_camera);
    }
    for(unsigned i=0;i<6;++i)psx_mod_write_word(SCRATCH+resource_offsets[i],resources[i]);
    etanks_project(0);
    projected=0;
}
static void leave_second(void) {leave_second_context(0);}
static int canonical_world_call(CPUState *cpu,uint32_t address,PSXModFunctionFilterCallback callback) {
    unsigned suspended=ai_suspended;
    ai_suspended=1;leave_second_context(1);world_owner=0;
    uint32_t result;
    if(callback(cpu,address))result=cpu->gpr[2];
    else result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    uint8_t campaign=psx_mod_read_byte(PLAY+0x44);enter_second();world_owner=1;
    psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
    ai_suspended=suspended;
    cpu->gpr[2]=result;return 1;
}
static int ai_canonical_call(CPUState *cpu,uint32_t address,PSXModFunctionFilterCallback callback) {
    if(!ai_call || !ai_owner || ai_suspended)return callback(cpu,address);
    return canonical_world_call(cpu,address,callback);
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
    psx_mod_write_word(diagnostic+0xA4,both_door.phase|((uint32_t)both_door.owner<<8));
    psx_mod_write_word(diagnostic+0xA8,both_door.actor);
    psx_mod_write_word(diagnostic+0xAC,(uint16_t)both_door.old_min|((uint32_t)(uint16_t)both_door.target_min<<16));
    psx_mod_write_word(diagnostic+0xB0,(uint16_t)both_door.target_max|((uint32_t)(uint16_t)both_door.right<<16));
    project(diagnostic+0x100,second.body,sizeof second.body);
    if(!projected) {
        capture(PLAYER,first.body,sizeof first.body);
        capture(GLOW,first.glow,sizeof first.glow);capture(TRAILS,first.trails,sizeof first.trails);
    }
    project(diagnostic+0x300,first.body,sizeof first.body);
    project(diagnostic+0x480,first.car,sizeof first.car);project(diagnostic+0x540,second.car,sizeof second.car);
    project(diagnostic+0xA00,first.glow,sizeof first.glow);project(diagnostic+0xB00,second.glow,sizeof second.glow);
    project(diagnostic+0xC00,first.trails,sizeof first.trails);project(diagnostic+0xD40,second.trails,sizeof second.trails);
    psx_mod_write_word(diagnostic+0xE80,personal_visual_ticks[0]);
    psx_mod_write_word(diagnostic+0xE84,personal_visual_ticks[1]);
    /* Observe private attacks at the same stable boundary as their bodies. */
    for(unsigned seat=0;seat<2;++seat) {
        const uint8_t *shots=seat?second.shots:first.shots;unsigned count=0;
        for(unsigned i=0;i<32;++i)if(shots[i*0x9C])++count;
        psx_mod_write_word(diagnostic+0xE88+seat*4,count);
    }
}
static void reset(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;if(busy || projected)return;
    /* Original 3BBA8/3BBC8..3BCE8 uses signed PLAY+1E == -1 for saved HP/ammo/weapon
     * continuation. A team wipe or campaign departure starts a fresh team. */
    unsigned same_stage=ever_enrolled && !stage_departed && !life.wipe &&
        last_stage==psx_mod_read_byte(PLAY+0x0C);
    unsigned withdrawn_forward=same_stage && psx_mod_read_byte(PLAY+0x0D)>last_section &&
        (life.status[1]==MMX5_COOP_ABSENT || life.status[1]==MMX5_COOP_LEAVING);
    /* Native forward areas use spawn policy0 and refill living/fallen heroes.
     * A deliberate withdrawal remains absent with its own resources, as in
     * X4. A new mission, campaign departure or wipe still starts a new team. */
    section_pending=same_stage && (psx_mod_read_byte(PLAY+0x1E)==0xFF || withdrawn_forward);
    /* Native Quit retires its stage/menu continuation without returning
     * through stage_tick's epilogue. Its recursion guard belongs to that
     * departed stage, not the newly initialized campaign or continuation. */
    enrolled=0;previous_input=0;stage_call=0;ride_deferred_slots=0;balance_reset();
    memset(&story_duel,0,sizeof story_duel);
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
    memset(personal_effect_owner,0,sizeof personal_effect_owner);
    memset(landing_effects,0,sizeof landing_effects);
    memset(car_source_owner,0,sizeof car_source_owner);
    ++asset_generation;
    join_phase=0;
    scene_owner=scene_phase=paired_victory=scene_corridor_arrow=scene_capsule=0;
    memset(&both_door,0,sizeof both_door);door_wait=door_traversal_call=door_terrain_call=0;
    pause_owner=-1;p2_start_previous=0;personal_menu_reset();
    diagnostics();
}
static void observe_stage(void) {
    unsigned mode=psx_mod_read_byte(PLAY);
    /* Native new-game/card-load frontend owns a new campaign. In particular,
     * original1CAA4..1CAAC clears stored E/W energy on a real card Load.
     * Preserve private contents through stage/section/rejoin, never over that reset. */
    if(mode<=1) {
        etanks_initialized=0;memset(private_tank_contents,0,sizeof private_tank_contents);
        duel_story_character=0;
        selected_x_armor=1;selected_zero_form=mmx5_coop_forms_requested(1);armor_selection_seen=0;
        armor_selecting_seat=armor_list_seat=0;memset(armor_previous_input,0,sizeof armor_previous_input);
        armor_roster_active=armor_native_count=0;memset(armor_native_roster,0,sizeof armor_native_roster);
    }
    /* Native mode 9 (203F0, including player init at 204E4) belongs to the
     * loader after mode 8. It must not turn a section continuation into a
     * fresh team merely because loading crossed a VBlank. */
    if(ever_enrolled && mode!=8 && mode!=9 && mode!=10)stage_departed=1;
#ifdef MMX5_COOP_TEST_CHEATS
    test_menu_commands();
#endif
}
#ifdef MMX5_COOP_TEST_CHEATS
/* Qualification commands use logical P1's replicated SIO packet. Never use
 * local-port selection or an online RAM mailbox: both peers must execute the
 * command once at the same canonical world boundary, outside render replay.
 * Hold L3+R3 and encode command 1..13 in the four shoulder button bits. */
static void test_receipt(unsigned command,uint32_t result) {
    ++test_commands;psx_mod_write_word(diagnostic+0x80,test_commands);
    psx_mod_write_word(diagnostic+0x84,command);psx_mod_write_word(diagnostic+0x88,result);
    psx_mod_counter_add("mmx5.coop.test-cheat",1);
}
static void test_stage_entry(unsigned command) {
    psx_mod_write_byte(PLAY+0x0C,(uint8_t)(command==7?5:command==8?4:1));
    psx_mod_write_byte(PLAY+0x0D,(uint8_t)(command==8?1:0));
    /* Original fresh-entry80020D0C/10 clears checkpoint and spawn policy.
     * Do not carry a previous native continuation's FF inventory restore. */
    psx_mod_write_byte(PLAY+0x1D,0);psx_mod_write_byte(PLAY+0x1E,0);
    for(unsigned i=1;i<4;++i)psx_mod_write_byte(PLAY+i,0);
    psx_mod_write_byte(PLAY,5);
}
static void test_menu_commands(void) {
    unsigned mode=psx_mod_read_byte(PLAY);
    if(!diagnostic || busy || projected || psx_mod_local_view_scope() || (mode!=4 && mode!=13))return;
    uint16_t held=(uint16_t)~sio_get_pad_buttons_slot(0);
    if((held&6)!=6) {test_chord=0;return;}
    unsigned command=(held>>8)&15;
    if(!test_chord && (command==7 || command==8 || command==12)) {
        /* Loaded selector/Game Over has campaign data but no world tick.
         * Only the original mission-entry mode writes are permitted here.
         * VBlank/SIO input is shared simulation state on both peers. */
        test_chord=1;test_stage_entry(command);test_receipt(command,1);
    }
}
static void test_place(unsigned seat,uint32_t x,uint32_t y) {
    if(seat==(world_owner?1u:0u)) {
        psx_mod_write_word(PLAYER+8,x);psx_mod_write_word(PLAYER+12,y);
        psx_mod_write_word(PLAYER+0x18,x);psx_mod_write_word(PLAYER+0x1C,y);
    } else {
        uint8_t *body=seat?second.body:first.body;
        for(unsigned i=0;i<4;++i) {
            body[8+i]=body[0x18+i]=(uint8_t)(x>>(8*i));
            body[12+i]=body[0x1C+i]=(uint8_t)(y>>(8*i));
        }
    }
}
static void test_fatal(unsigned seat) {
    test_god_mask&=~(1u<<seat);
    if(seat==(world_owner?1u:0u))psx_mod_write_byte(PLAYER+0x5C,0x80);
    else (seat?second.body:first.body)[0x5C]=0x80;
    if(car_context()) {
        if(seat==(world_owner?1u:0u))psx_mod_write_byte(CAR+0x5C,0);
        else (seat?second.car:first.car)[0x5C]=0;
    }
}
static void test_commands_tick(CPUState *cpu) {
    if(busy || (projected && !world_owner) || local_view_call)return;
    uint16_t held=(uint16_t)~sio_get_pad_buttons_slot(0);
    unsigned chord=(held&6)==6,command=(held>>8)&15,result=0;
    if(chord && !test_chord) {
        if((command==1 || command==2) && psx_mod_read_byte(PLAY+0x0C)==1 &&
            !psx_mod_read_byte(PLAY+0x0D) && !scene_owner) {
            /* Original loader resolves the checkpoint record, camera bounds,
             * enemy placements and arrival. Do not fabricate a victory flag. */
            psx_mod_write_byte(PLAY+0x1D,(uint8_t)(command==1?1:4));
            psx_mod_write_byte(PLAY+0x0F,0xFF);guest(cpu,0x800205AC,PLAY,0);result=1;
        } else if(command==3 && psx_mod_read_byte(PLAY+0x0C)==1 &&
            psx_mod_read_word(0x800EE970)==5) {
            for(unsigned i=0;i<48;++i) {
                uint32_t actor=0x80092090u+i*0x9Cu;
                if(psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==0x36 &&
                    psx_mod_read_byte(actor+4)==1 && psx_mod_read_byte(actor+5)!=9 &&
                    psx_mod_read_byte(actor+0x5C)>0 && psx_mod_read_byte(actor+0x5C)<128) {
                    /* BIN1 F880C..F8828 enters death on a negative damage
                     * return, not an HP poll. Reproduce that actor transition;
                     * original F9D00 drives the death pose/effects/controller.
                     * No campaign, reward or victory flag is written here. */
                    psx_mod_write_byte(actor+0x5C,0x80);
                    psx_mod_write_byte(PLAYER+0x7A,1);
                    psx_mod_write_byte(actor+4,2);psx_mod_write_byte(actor+5,0);
                    result=actor;break;
                }
            }
        } else if(command==4) {test_god_mask=3;result=1;}
        else if(command==5 || command==6) {test_fatal(command-5);result=1;}
        else if(command==7 || command==8 || command==12) {
            /* Use original mission title/loader, including selected mission
             * +26/+27. No disc code or actor is copied into guest memory. */
            test_stage_entry(command);result=1;
        } else if((command==9 || command==10) && native_armor_overlay()) {
            if(psx_mod_read_byte(CAR) && psx_mod_read_byte(CAR+1)==1) {
                test_place(command-9,psx_mod_read_word(CAR+8),psx_mod_read_word(CAR+12)-16*65536u);
                result=1;
            }
        } else if(command==11 && life.status[0]==MMX5_COOP_ALIVE &&
            life.status[1]==MMX5_COOP_ALIVE && !car_context() && !scene_owner) {
            uint32_t x=world_owner?le32(first.body+8):psx_mod_read_word(PLAYER+8);
            uint32_t y=world_owner?le32(first.body+12):psx_mod_read_word(PLAYER+12);
            test_place(1,x+(24u<<16),y);result=1;
        } else if(command==13) {
            psx_mod_write_byte(PLAY+0x45,3);result=1;
        }
        test_receipt(command,result);
    }
    test_chord=chord;
    /* Refill HP only. Native A4/61 immunity also rejects Armor contacts;
     * changing those would conceal the native mount/door paths being tested. */
    for(unsigned seat=0;seat<2;++seat)if((test_god_mask&(1u<<seat)) &&
        life.status[seat]==MMX5_COOP_ALIVE) {
        unsigned resident=seat==(world_owner?1u:0u);
        unsigned character=resident?psx_mod_read_byte(PLAYER+2):(seat?second.body:first.body)[2];
        /* BODY+5D is the animated displayed HP, not capacity. The native HUD
         * reads the character maximum from PLAY+47/+48 (80026310..80026324). */
        uint8_t maximum=psx_mod_read_byte(PLAY+0x47+(character&1u));
        if(seat==(world_owner?1u:0u)) {
            if(!(psx_mod_read_byte(PLAYER+0x5C)&128)) {
                psx_mod_write_byte(PLAYER+0x5C,maximum);
            }
        } else {
            uint8_t *body=seat?second.body:first.body;
            if(!(body[0x5C]&128))body[0x5C]=maximum;
        }
        if(car_context() && !((seat==(world_owner?1u:0u)?psx_mod_read_byte(PLAYER+0x5C):
            (seat?second.body:first.body)[0x5C])&128)) {
            if(seat==(world_owner?1u:0u))psx_mod_write_byte(CAR+0x5C,maximum);
            else (seat?second.car:first.car)[0x5C]=maximum;
        }
    }
    /* Explicit pickup/menu setup protection; never qualify damage or mounts
     * while it is held. The native short timer expires after disabling it. */
    for(unsigned seat=0;seat<2;++seat)if((test_contact_mask&(1u<<seat)) &&
        life.status[seat]==MMX5_COOP_ALIVE) {
        if(seat==(world_owner?1u:0u)) {
            psx_mod_write_byte(PLAYER+0xA4,1);psx_mod_write_byte(PLAYER+0x61,2);
        }else {
            uint8_t *body=seat?second.body:first.body;body[0xA4]=1;body[0x61]=2;
        }
    }
}
#endif
static void world_tick(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(personal_menu_call && personal_menu_seat)return;
#ifdef MMX5_COOP_TEST_CHEATS
    test_commands_tick(cpu);
#endif
#ifndef PSX_NO_DEBUG_TOOLS
    fixture_scenery_begin(cpu,address);
    if(fixture_damage_actor && (psx_mod_read_byte(PLAY)!=10 ||
       psx_mod_read_byte(PLAY+12)!=fixture_damage_stage))fixture_damage_actor=0;
    if(diagnostic && !psx_mod_netplay_is_active()) {
        uint32_t sequence=psx_mod_read_word(diagnostic+0x50);
        if(sequence!=fixture_sequence) {
            unsigned command=psx_mod_read_byte(diagnostic+0x54),seat=psx_mod_read_byte(diagnostic+0x55)&1;
            mmx5_diagnostics_development_fixture(command,seat,psx_mod_read_byte(diagnostic+0x56),
                                                psx_mod_read_half(diagnostic+0x58));
            unsigned resident=world_owner?1:0;
            psx_mod_write_word(diagnostic+0x64,command>=1 && command<=8);
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
            if(command==20 && enrolled && !failed && !scene_owner &&
               !psx_mod_read_byte(PLAY+1) && !psx_mod_read_byte(PLAY+0x1C)) {
                /* Offline forced damage: the next original boss shot query
                 * receives a lethal hit. Its caller still owns death entry,
                 * effects, rewards, module changes and actual progression. */
                uint32_t actor=psx_mod_read_word(PLAY+0x20);
                if(actor>=0x80092090u && actor<0x80093DD0u &&
                   !((actor-0x80092090u)%0x9Cu) && psx_mod_read_byte(actor) &&
                   psx_mod_read_byte(actor+1)>=0x31u &&
                   psx_mod_read_byte(actor+1)<=0x43u && psx_mod_read_byte(actor+4)==1 &&
                   (psx_mod_read_byte(actor+0x5C)&127u)) {
                    fixture_damage_actor=actor;fixture_damage_kind=psx_mod_read_byte(actor+1);
                    fixture_damage_stage=psx_mod_read_byte(PLAY+12);
                    psx_mod_write_word(diagnostic+0x64,1);
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
                if(separate_etanks) {
                    etanks_seed();private_tank_contents[seat][0]=psx_mod_read_byte(diagnostic+0x56)&0x7Fu;
                    if(seat==resident)etanks_project(seat);
                } else psx_mod_write_byte(PLAY+0x76,psx_mod_read_byte(diagnostic+0x56));
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
            if((command==9 || command==14) && !scene_owner && !psx_mod_read_byte(PLAY+1)) {
                unsigned stage=psx_mod_read_byte(diagnostic+0x56);
                unsigned checkpoint=psx_mod_read_byte(diagnostic+0x58);
                unsigned section=psx_mod_read_byte(diagnostic+0x59);
                int same_area=stage==psx_mod_read_byte(PLAY+0x0C) &&
                    section==psx_mod_read_byte(PLAY+0x0D) &&
                    life.status[0]==MMX5_COOP_ALIVE && life.status[1]==MMX5_COOP_ALIVE;
                if(fixture_location_valid(stage,section,checkpoint) && (command==9 || same_area)) {
                    fixture_stage=stage;fixture_section=section;fixture_checkpoint=checkpoint;
                    fixture_location_pending=1;
                    if(command==14) {
                        /* Reuse original same-area continuation for dense QA
                         * coverage. No mission title or success flag is set. */
                        psx_mod_write_byte(PLAY+0x0F,0xFF);guest(cpu,0x800205AC,PLAY,0);
                    }else {
                        /* Stage fields stay valid for the rest of this world
                         * update; fixture_mission_begin applies them at5. */
                        for(unsigned i=1;i<4;++i)psx_mod_write_byte(PLAY+i,0);
                        psx_mod_write_byte(PLAY,5);
                    }
                    psx_mod_write_word(diagnostic+0x64,1);
                }
            }
            if(command==18 && enrolled && !failed && !busy && !scene_owner &&
               (!projected || world_owner) && !psx_mod_local_view_scope() &&
               psx_mod_read_byte(PLAY)==10 && !psx_mod_read_byte(PLAY+1) &&
               psx_mod_read_byte(PLAY+12)==8 && !psx_mod_read_byte(PLAY+13) &&
               psx_mod_read_byte(PLAY+0x1D)>=2 && psx_mod_read_byte(PLAY+0x1D)<=4) {
                /* Retained checkpoint fixtures can also predate the scenery
                 * prerequisite fix. Replay only the same guarded original
                 * door06 loading request; no later door/room result is set. */
                fixture_scenery_pending=1;fixture_scenery_begin(cpu,0);
                unsigned request=psx_mod_read_word(0x800E92F8u);
                psx_mod_write_word(diagnostic+0x64,request==1 || request==2);
            }
            if(command==10 && !scene_owner && life.status[seat]==MMX5_COOP_ALIVE) {
                /* Change position/history only: native gravity, collision,
                 * controls, triggers and success flags still run normally. */
                for(unsigned i=0;i<8;++i) {
                    uint8_t value=psx_mod_read_byte(diagnostic+0x90+i);
                    if(seat==resident) {
                        psx_mod_write_byte(PLAYER+8+i,value);psx_mod_write_byte(PLAYER+0x18+i,value);
                    }else {
                        uint8_t *body=seat?second.body:first.body;
                        body[8+i]=body[0x18+i]=value;
                    }
                }
                psx_mod_write_word(diagnostic+0x64,1);
            }
            if(command==19 && split_available() && !busy && !scene_owner && !car_context() &&
               !psx_mod_read_byte(PLAY+1) && !psx_mod_read_byte(PLAY+0x0F) &&
               !psx_mod_read_byte(PLAY+0x10) && !psx_mod_read_byte(PLAY+0x1C) &&
               both_living(seat) && !both_body_byte(seat,0xD0) &&
               !both_body_byte(seat,0xD4) && !both_body_byte(seat,0xD5)) {
                unsigned checkpoint=psx_mod_read_byte(diagnostic+0x56);
                unsigned stage=psx_mod_read_byte(PLAY+12),section=psx_mod_read_byte(PLAY+13);
                if(fixture_location_valid(stage,section,checkpoint)) {
                    /* Independent offline positioning at an actual native
                     * checkpoint record. This bypasses traversal only: no
                     * checkpoint, room, actor, reward or transition flag is
                     * changed. Native physics and placement admission resume.
                     * Unlike paired16, it can prepare two authored rooms with
                     * disjoint bounds for camera/health ownership checks. */
                    uint32_t table=psx_mod_read_word(0x80072D10u+(stage*2u+section)*4u);
                    uint32_t record=psx_mod_read_word(table+checkpoint*4u);
                    for(unsigned axis=0;axis<2;++axis) {
                        uint32_t value=(uint32_t)((int16_t)psx_mod_read_half(record+axis*2u)*65536);
                        both_body_word_write(seat,8+axis*4,value);
                        both_body_word_write(seat,0x18+axis*4,value);
                    }
                    uint8_t *view=split_cameras[seat];
                    for(unsigned axis=0;axis<2;++axis) {
                        uint32_t value=(uint32_t)((int16_t)psx_mod_read_half(record+4+axis*2u)*65536);
                        put32(view+8+axis*4,value);put32(view+0x14+axis*4,value);
                    }
                    const unsigned source[4]={0x12,0x10,0x16,0x14};
                    for(unsigned set=0;set<2;++set)for(unsigned bound=0;bound<4;++bound) {
                        unsigned offset=0x1C+set*8+bound*2;
                        uint16_t value=psx_mod_read_half(record+source[bound]);
                        view[offset]=(uint8_t)value;view[offset+1]=(uint8_t)(value>>8);
                    }
                    view[0x44]=1;
                    if(seat==resident) {project(CAMERA,view,sizeof saved_camera);split_owner=seat+1;}
                    psx_mod_write_word(diagnostic+0x64,1);
                }
            }
            if(command==16 && enrolled && !failed && !busy && (!projected || world_owner) &&
               !psx_mod_local_view_scope() && !scene_owner && !car_context() &&
               psx_mod_read_byte(PLAY)==10 && !psx_mod_read_byte(PLAY+1) &&
               !psx_mod_read_byte(PLAY+0x0F) && !psx_mod_read_byte(PLAY+0x10) &&
               !psx_mod_read_byte(PLAY+0x1C) && life.status[0]==MMX5_COOP_ALIVE &&
               life.status[1]==MMX5_COOP_ALIVE) {
                /* Offline paired setup at one native world boundary. Keep
                 * gravity, contacts, controls, scene/progression flags and
                 * every authored camera bound; this bypasses traversal.
                 * Payload: bodies+90..9F, camera X+A0/Y+B4. Existing
                 * diagnostics continuously owns +A4, so it is not payload. */
                int allowed=1;
                for(unsigned target=0;target<2;++target) {
                    const uint8_t *body=target?second.body:first.body;
                    unsigned state=target==resident?psx_mod_read_byte(PLAYER+4):body[4];
                    if(state!=1)allowed=0;
                    const unsigned locks[4]={0xD0,0xD4,0xD5,0x7A};
                    for(unsigned i=0;i<4;++i)
                        if(target==resident?psx_mod_read_byte(PLAYER+locks[i]):body[locks[i]])allowed=0;
                }
                int32_t x=(int32_t)psx_mod_read_word(diagnostic+0xA0);
                int32_t y=(int32_t)psx_mod_read_word(diagnostic+0xB4);
                uint8_t current[0xFC];capture(CAMERA,current,sizeof current);
                unsigned views=split_views && split_valid?3u:1u;
                for(unsigned view=0;view<views;++view) {
                    const uint8_t *camera=view?split_cameras[view-1]:current;
                    /* Current bounds can still enforce an interpolating
                     * encounter lock even after its target bounds change. */
                    for(unsigned bound_set=0;bound_set<2;++bound_set) {
                        unsigned base=0x1Cu+bound_set*8u;
                        int min_x=(int16_t)(camera[base+2]|camera[base+3]<<8);
                        int max_x=(int16_t)(camera[base]|camera[base+1]<<8);
                        int min_y=(int16_t)(camera[base+6]|camera[base+7]<<8);
                        int max_y=(int16_t)(camera[base+4]|camera[base+5]<<8);
                        if(x<min_x*65536 || x>max_x*65536 ||
                           y<min_y*65536 || y>max_y*65536)allowed=0;
                    }
                }
                if(allowed) {
                    for(unsigned target=0;target<2;++target)for(unsigned i=0;i<8;++i) {
                        uint8_t value=psx_mod_read_byte(diagnostic+0x90+target*8+i);
                        if(target==resident) {
                            psx_mod_write_byte(PLAYER+8+i,value);
                            psx_mod_write_byte(PLAYER+0x18+i,value);
                        }else {
                            uint8_t *body=target?second.body:first.body;
                            body[8+i]=body[0x18+i]=value;
                        }
                    }
                    for(unsigned i=0;i<8;++i) {
                        unsigned offset=i<4?0xA0u:0xB4u;
                        uint8_t value=psx_mod_read_byte(diagnostic+offset+(i&3u));
                        psx_mod_write_byte(CAMERA+8+i,value);
                        psx_mod_write_byte(CAMERA+0x14+i,value);
                        /* camera_update restores these origins before native
                         * motion. Preserve each cache's existing bounds and
                         * leave native secondary/parallax layers untouched. */
                        if(split_views && split_valid)for(unsigned target=0;target<2;++target)
                            split_cameras[target][8+i]=split_cameras[target][0x14+i]=value;
                    }
                    psx_mod_write_word(diagnostic+0x64,1);
                }
            }
            if(command==17 && enrolled && !failed && !busy && (!projected || world_owner) &&
               !psx_mod_local_view_scope() && !scene_owner &&
               psx_mod_read_byte(PLAY)==10 && !psx_mod_read_byte(PLAY+1) &&
               !psx_mod_read_byte(PLAY+0x0F) && !psx_mod_read_byte(PLAY+0x10) &&
               (!psx_mod_read_byte(PLAY+0x1C) || car_context())) {
                /* Offline content setup: a stationary teleport bypasses the
                 * native camera-edge scanner. Ask ORIGINAL 2B070 to scan a
                 * small authored region, retaining its allocation/cooldown
                 * and already-visited guards. No actor/grant/scene state is
                 * fabricated. Ride Chaser keeps its pause-input gate set
                 * during ordinary driving; that gate must not reject the
                 * same bounded placement setup on an otherwise live road.
                 * The surviving P2 is already canonical in its
                 * world-owner scope. This does not qualify viewport entry. */
                int32_t left=(int32_t)psx_mod_read_word(diagnostic+0x90);
                int32_t right=(int32_t)psx_mod_read_word(diagnostic+0x94);
                int32_t top=(int32_t)psx_mod_read_word(diagnostic+0x98);
                int32_t bottom=(int32_t)psx_mod_read_word(diagnostic+0x9C);
                if(left>=0 && top>=0 && right>=left && bottom>=top &&
                   right<=32767 && bottom<=32767 && right-left<=512 && bottom-top<=480) {
                    PSXPlacementRect region={left,right,top,bottom};
                    split_scan_rectangle(cpu,region);
                    psx_mod_write_word(diagnostic+0x64,1);
                }
            }
#ifdef MMX5_COOP_TEST_CHEATS
            if(command==15 && !scene_owner && life.status[seat]==MMX5_COOP_ALIVE && car_context()) {
                /* Offline road-position fixture. Move the existing car and
                 * rider histories together; native velocity, contacts, input,
                 * hazards, scene clocks and retry/success flags stay intact. */
                uint8_t *car=seat?second.car:first.car;
                int mounted=seat==resident?psx_mod_read_byte(PLAYER+0xD5)==0xFF:
                    (seat?second.body:first.body)[0xD5]==0xFF;
                if(mounted) {
                    for(unsigned i=0;i<8;++i) {
                        uint8_t value=psx_mod_read_byte(diagnostic+0x90+i);
                        if(seat==resident) {
                            psx_mod_write_byte(CAR+8+i,value);psx_mod_write_byte(CAR+0x18+i,value);
                            psx_mod_write_byte(PLAYER+8+i,value);psx_mod_write_byte(PLAYER+0x18+i,value);
                        }else {
                            uint8_t *body=seat?second.body:first.body;
                            car[8+i]=car[0x18+i]=value;body[8+i]=body[0x18+i]=value;
                        }
                    }
                    psx_mod_write_word(diagnostic+0x64,1);
                }
            }
            if(command==11) {
                test_god_mask=psx_mod_read_byte(diagnostic+0x56)&3u;
                psx_mod_write_word(diagnostic+0x64,1);
            }
            if(command==12) {
                test_contact_mask=psx_mod_read_byte(diagnostic+0x56)&3u;
                psx_mod_write_word(diagnostic+0x64,1);
            }
#endif
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
    prepare_private_x();
    int survivor_incoming=section_pending && life.status[0]==MMX5_COOP_FALLEN &&
        life.status[1]==MMX5_COOP_ALIVE && !car_context();
    int native_arrival=life.status[1]==MMX5_COOP_ALIVE && !car_context();
    uint32_t x=psx_mod_read_word(PLAYER+8),y=psx_mod_read_word(PLAYER+12);
    uint8_t inventory[0x158];memcpy(inventory,second.body,sizeof inventory);
    memset(&second,0,sizeof second);
    enter_second();
    if(!survivor_incoming)psx_mod_write_byte(PLAY+0x1E,0);
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
    psx_mod_write_byte(PLAYER+3,1);psx_mod_write_byte(PLAYER+4,(uint8_t)!native_arrival);
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
        /* The copied bike also needs its own native exhaust follower.
         * EEE18 normally creates it once during the global arrival, which
         * this private enrollment deliberately does not replay. */
        guest(cpu,0x800F0B24u,CAR,0);
        psx_mod_counter_add("mmx5.coop.chaser-enrolled",1);
    } else if(native_arrival) {
        /* Original3C0EC selects the checkpoint's actual incoming/transfer
         * root from PLAY1E/1D, increments constructor state0 and initializes
         * its personal gate/velocity. Action0 without that factory is inert. */
        psx_mod_write_byte(PLAYER+0xCE,first.body[0xCE]);
        /* Construction suppresses the singleton continuation cache, but
         * each hero still needs its own original incoming controller. Let
         * Zero finish its shorter native animation independently of X. */
        if(!psx_mod_read_byte(PLAY+0x1E))
            psx_mod_write_byte(PLAY+0x1E,saved_play[0x1E]?saved_play[0x1E]:1);
        guest(cpu,0x8003C0ECu,PLAYER,0);
        unsigned stage=psx_mod_read_byte(PLAY+0x0C);
        if(stage==9 || stage==10) {
            /* Dynamo's native selector hands the already visible story
             * actor directly to idle through 3C20C. The counterpart has no
             * story actor to hand over, so retain that native setup and run
             * the ordinary beam/landing controller instead of appearing idle. */
            guest(cpu,0x8003D72Cu,PLAYER,0);
            psx_mod_counter_add("mmx5.coop.dynamo-counterpart-arrivals",1);
        }
    } else {
        guest(cpu,0x8003D5E0,PLAYER,0);guest(cpu,0x8003B65C,PLAYER,0);guest(cpu,0x8003A670,PLAYER,0);
    }
    /* A sole survivor is the native arriving hero, not a passenger of the
     * hidden corpse. Keep3BAC8's action0 and let its original arrival root
     * establish parts, beam, landing and collision at the new checkpoint. */
    if(!survivor_incoming) {
        psx_mod_write_byte(PLAYER+0x14,first.body[0x14]);
        psx_mod_write_byte(PLAYER+0x15,first.body[0x15]);
    }
    if(!native_arrival)guest(cpu,0x8002FB58,PLAYER,0);
    leave_second();
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
    last_section=psx_mod_read_byte(PLAY+0x0D);
    memset(&dinorex_sequence,0,sizeof dinorex_sequence);
    memset(&glow_stair,0,sizeof glow_stair);
    enrolled=1;psx_mod_counter_add("mmx5.coop.enrolled",1);
    /* Enrollment can follow this frame's camera update. Seed both private
     * views now so the first arrival/HUD frame can already publish a pair. */
    if(split_views && !split_valid) {
        capture(CAMERA,split_cameras[0],sizeof saved_camera);
        memcpy(split_cameras[1],split_cameras[0],sizeof saved_camera);
        split_valid=1;
    }
    /* A stage constructor is independent of Select's optional join. The
     * native controller owns that character's beam, terrain and input release;
     * retained absent/fallen players and vehicle arrivals stay separate. */
    if(native_arrival && !survivor_incoming) {
        hud_fade=0;join_phase=0;
        psx_mod_counter_add("mmx5.coop.stage-arrivals",1);
    }
    return 1;
}
static uint16_t input(unsigned seat) {
    uint16_t buttons=sio_get_pad_buttons_slot((int)seat);
    mmx5_diagnostics_resolve_input(seat,&buttons);
    uint16_t raw=(uint16_t)~buttons;
    return (uint16_t)((raw<<8)|(raw>>8));
}
#include "mmx5_coop_armor.inc"
#include "mmx5_coop_bindings.inc"

static int native_armor_overlay(void) {
    /* BIN member 6 (logical ID 10), stage 4/section 1. These VMAs contain
     * unrelated code in other overlays. Never call them on address alone. */
    return psx_mod_read_word(0x800EE970)==10 &&
        psx_mod_read_word(0x800F2A4C)==0x27BDFFE8u &&
        psx_mod_read_word(0x800F2A58)==0x3C02800Au &&
        psx_mod_read_word(0x800EF170)==0x27BDFFE0u;
}
static int ride_factory(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(ride_spawn_call || failed || psx_mod_local_view_scope() || !extra_ride_record ||
       psx_mod_read_byte(PLAY+12)!=4 || psx_mod_read_byte(PLAY+13)!=1 ||
       !native_armor_overlay() || address!=0x800EE988u ||
       psx_mod_read_word(address)!=0x27BDFFE8u ||
       actor<0x8009E7A0u || actor>=0x8009F9A0u || (actor-0x8009E7A0u)%0x30u ||
       psx_mod_read_byte(actor+1)!=0x1E || psx_mod_read_byte(actor+2)>1)return 0;
    unsigned slot=psx_mod_read_byte(actor+2);
    if(psx_mod_read_byte(CAR+slot*0xB0u)) {
        /* The authored late backup cannot overwrite an occupied early suit.
         * Retire only its original placeholder through the native program. */
        cpu->gpr[2]=guest(cpu,0x8002D720u,actor,0);return 1;
    }
    uint32_t source=psx_mod_read_word(actor+0x10);uint8_t placement[0x30];
    capture(actor,placement,sizeof placement);ride_spawn_call=1;
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    if(!slot && source==0x800FDBCCu && !psx_mod_read_byte(CAR+0xB0u) &&
       (psx_mod_read_word(source)&0xFFFFFF00u)==0x03001E00u &&
       mmx5_coop_zero_available(psx_mod_read_byte(PLAY+0x79),psx_mod_read_byte(PLAY+0xCB),keep_zero)) {
        /* Reuse the just-retired native placeholder, not an initialized
         * vehicle with aliased pointers. Each suit gets its original native
         * initializer, physical slot and distinct placement source. The real
         * late backup remains available if this extra suit is lost earlier. */
        /* The starting platform ends at X160, with its top at Y384.
         * Copying the lava-side Y411 into the extra suit at X232 embedded
         * its boarding box in the scenery. Keep its full native bounds on
         * the platform and clear of the X96/X120 arrival positions, so entry
         * remains the player's choice. Native gravity settles it at Y352. */
        uint32_t x=le32(placement+8)-232u*65536u;
        uint32_t y=le32(placement+12)-80u*65536u;
        for(unsigned i=0;i<8;++i)psx_mod_write_byte(extra_ride_record+i,psx_mod_read_byte(source+i));
        psx_mod_write_byte(extra_ride_record,0);psx_mod_write_byte(extra_ride_record+2,1);
        psx_mod_write_half(extra_ride_record+4,(uint16_t)(x>>16));
        psx_mod_write_half(extra_ride_record+6,(uint16_t)(y>>16));
        project(actor,placement,sizeof placement);psx_mod_write_byte(actor+2,1);
        psx_mod_write_word(actor+8,x);psx_mod_write_word(actor+12,y);
        psx_mod_write_word(actor+0x10,extra_ride_record);
        (void)guest(cpu,address,actor,0);
        psx_mod_counter_add("mmx5.coop.second-armor-spawns",1);
    }
    ride_spawn_call=0;cpu->gpr[2]=result;return 1;
}
static unsigned ride_slot_owner(unsigned slot) {
    unsigned owner=slot,best=UINT32_MAX;
    int x=(int16_t)psx_mod_read_half(CAR+slot*0xB0u+10);
    int y=(int16_t)psx_mod_read_half(CAR+slot*0xB0u+14);
    for(unsigned seat=0;seat<2;++seat) {
        const uint8_t *body=seat?second.body:first.body;
        int resident=seat==(unsigned)(projected!=0);
        unsigned mounted=resident?psx_mod_read_byte(PLAYER+0xD5):body[0xD5];
        /* Native39AC8 changes its pending slot1/2 marker into FF/FE.
         * Follow both phases of that real rider instead of assigning suits
         * by controller number. Occupied suits never change owner by proximity. */
        if(mounted==slot+1u || mounted==255u-slot)return seat;
        if(mounted || life.status[seat]!=MMX5_COOP_ALIVE || scene_passenger(seat) ||
           (resident?psx_mod_read_byte(PLAYER+4):body[4])!=1)continue;
        int bx=resident?(int16_t)psx_mod_read_half(PLAYER+10):(int16_t)(le32(body+8)>>16);
        int by=resident?(int16_t)psx_mod_read_half(PLAYER+14):(int16_t)(le32(body+12)>>16);
        int dx=x-bx,dy=y-by;
        unsigned distance=(unsigned)((dx<0?-dx:dx)+(dy<0?-dy:dy));
        if(distance<best || (distance==best && seat==slot)) {owner=seat;best=distance;}
    }
    return owner;
}
static uint32_t ride_native_slot(CPUState *cpu,uint32_t address,unsigned seat,unsigned slot) {
    uint32_t other=CAR+(slot^1u)*0xB0u;uint8_t active=psx_mod_read_byte(other);
    uint8_t pad[6];uint32_t box=psx_mod_read_word(PLAYER+0x68);
    int unavailable=life.status[seat]!=MMX5_COOP_ALIVE || scene_passenger(seat);
    int no_mount=unavailable && !(psx_mod_read_byte(CAR+slot*0xB0u+0x97)&0x40u);
    if(unavailable) {capture(PLAYER+0x7C,pad,sizeof pad);for(unsigned i=0;i<sizeof pad;++i)psx_mod_write_byte(PLAYER+0x7C+i,0);}
    if(no_mount)psx_mod_write_word(PLAYER+0x68,0);
    psx_mod_write_byte(other,0);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    psx_mod_write_byte(other,active);
    if(no_mount)psx_mod_write_word(PLAYER+0x68,box);
    if(unavailable)project(PLAYER+0x7C,pad,sizeof pad);
    return result;
}
static uint32_t ride_body_scope(CPUState *cpu,uint32_t address,unsigned seat,unsigned slot) {
    if(seat==world_owner)return ride_native_slot(cpu,address,seat,slot);
    if(seat) {
        uint8_t campaign=psx_mod_read_byte(PLAY+0x44);enter_second();world_owner=1;
        psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
        uint32_t result=ride_native_slot(cpu,address,1,slot);
        world_owner=0;leave_second_context(1);return result;
    }
    /* The actual P1 rider retains its suit during P2 world ownership. */
    capture_player(&second);project_player(&first);
    uint32_t pointers[6];
    uint8_t character=psx_mod_read_byte(PLAY+0x44),roster=psx_mod_read_byte(PLAY+0x49);
    psx_mod_write_byte(PLAY+0x44,saved_play[0x44]);psx_mod_write_byte(PLAY+0x49,saved_play[0x49]);
    for(unsigned i=0;i<6;++i) {
        pointers[i]=psx_mod_read_word(SCRATCH+resource_offsets[i]);
        psx_mod_write_word(SCRATCH+resource_offsets[i],resources[i]);
    }
    world_owner=0;
    uint32_t result=ride_native_slot(cpu,address,0,slot);
    world_owner=1;
    capture_player(&first);project_player(&second);
    for(unsigned i=0;i<6;++i)psx_mod_write_word(SCRATCH+resource_offsets[i],pointers[i]);
    psx_mod_write_byte(PLAY+0x44,character);psx_mod_write_byte(PLAY+0x49,roster);
    psx_mod_counter_add("mmx5.coop.p1-armor-world-projections",1);return result;
}
static int ride_second_deferred(void) {
    /* Match the private body's active gameplay boundary. Scenes and inactive
     * seats retain the original shared dispatcher rather than missing a tick
     * or advancing the same physical suit in both passes. */
    return !world_owner && life.status[1]==MMX5_COOP_ALIVE && !scene_owner &&
        !psx_mod_read_byte(PLAY+0x10) &&
        (!psx_mod_read_byte(PLAY+0x1C) || personal_giga());
}
static int ride_tick(CPUState *cpu,uint32_t address) {
    if(ride_call || busy || !enrolled || failed || psx_mod_local_view_scope() || !native_armor_overlay())return 0;
    ride_call=1;ride_deferred_slots=0;uint32_t result=0;
    for(unsigned slot=0;slot<2;++slot) {
        unsigned seat=ride_slot_owner(slot);
        /* An active P2 gets the same input -> vehicle -> body ordering as
         * P1, at second_tick's existing private-player boundary below. */
        if(seat==1 && ride_second_deferred()) {ride_deferred_slots|=1u<<slot;continue;}
        uint8_t camera[0xFC];unsigned camera_owner=split_owner,previous_scene=scene_owner;
        int private_camera=split_views && split_valid && seat!=world_owner;
        if(private_camera) {capture(CAMERA,camera,sizeof camera);project(CAMERA,split_cameras[seat],sizeof camera);split_owner=seat+1;}
        /* The native pool dispatcher keeps its scene/freeze policy. Mask the
         * other physical slot for this call so every suit advances once,
         * against its actual rider, input and camera. */
        result=ride_body_scope(cpu,address,seat,slot);
        if(private_camera) {
            capture(CAMERA,split_cameras[seat],sizeof camera);
            if(scene_owner!=seat+1 || scene_owner==previous_scene) {project(CAMERA,camera,sizeof camera);split_owner=camera_owner;}
        }
        if(seat)psx_mod_counter_add("mmx5.coop.p2-armor-world-ticks",1);
    }
    ride_call=0;cpu->gpr[2]=result;return 1;
}
static void ride_second_tick(CPUState *cpu) {
    if(!native_armor_overlay() || ride_call || !ride_deferred_slots || !ride_second_deferred())return;
    unsigned slots=ride_deferred_slots;ride_deferred_slots=0;
    uint8_t camera[0xFC],campaign=psx_mod_read_byte(PLAY+0x44);
    uint32_t palette=psx_mod_read_word(SCRATCH+0x28);unsigned camera_owner=split_owner;
    int private_camera=split_views && split_valid;
    if(private_camera) {capture(CAMERA,camera,sizeof camera);project(CAMERA,split_cameras[1],sizeof camera);split_owner=2;}
    psx_mod_write_byte(PLAY+0x44,saved_play[0x44]);psx_mod_write_word(SCRATCH+0x28,resources[3]);
    ride_call=1;world_owner=1;
    for(unsigned slot=0;slot<2;++slot)if(slots&(1u<<slot))
        (void)ride_native_slot(cpu,0x80022960u,1,slot);
    world_owner=0;ride_call=0;
    psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,palette);
    if(private_camera) {capture(CAMERA,split_cameras[1],sizeof camera);project(CAMERA,camera,sizeof camera);split_owner=camera_owner;}
    psx_mod_counter_add("mmx5.coop.p2-armor-world-ticks",1);
}
static void put32(uint8_t *p,uint32_t value) {
    for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(value>>(i*8));
}
static void clear_personal_drawables(unsigned seat) {
    PlayerContext *context=seat?&second:&first;
    int resident=seat==(unsigned)(projected!=0);
    for(unsigned i=0;i<3;++i)for(unsigned offset=3;offset<=4;++offset) {
        /* These are permanent native slots with initialized type/assembly
         * fields. Hide/reset their state without destroying the templates
         * needed by the next native charge or dash after a rejoin/scene. */
        if(resident) {
            psx_mod_write_byte(GLOW+i*0x50u+offset,0);
            psx_mod_write_byte(TRAILS+i*0x60u+offset,0);
        } else {
            context->glow[i*0x50u+offset]=0;context->trails[i*0x60u+offset]=0;
        }
    }
}
static void clear_buster_effects(unsigned seat) {
    for(unsigned i=0;i<96;++i) {
        uint32_t actor=0x80094398u+i*0x70u;
        if(personal_effect_owner[i]==seat && psx_mod_read_byte(actor) &&
           psx_mod_read_byte(actor+1)==2) {
            /* Shared delayed emitters outlive the private projectile pool.
             * Match X4's attack reset: an outgoing/dead passenger must not
             * allocate a new shot later in the shared effect dispatcher. */
            psx_mod_write_byte(actor,0);psx_mod_write_byte(actor+3,0);
            personal_effect_owner[i]=0;
            psx_mod_counter_add(seat?"mmx5.coop.p2-buster-effects-cleared":
                                    "mmx5.coop.p1-buster-effects-cleared",1);
        }
    }
}
static void start_leave(CPUState *cpu) {
    clear_buster_effects(1);
    memcpy(returning_body,second.body,sizeof returning_body);
    memset(second.shots,0,sizeof second.shots);clear_personal_drawables(1);
    enter_second();guest(cpu,0x8003A2C8,PLAYER,0);guest(cpu,0x8003B33C,PLAYER,3);
    guest(cpu,0x8003B28C,PLAYER,0);psx_mod_write_word(PLAYER+0x54,0);leave_second();
}
static void begin_join(CPUState *cpu,uint32_t x,uint32_t y) {
    uint32_t leader_box=psx_mod_read_word(PLAYER+0x68);
    uint8_t facing[2]={psx_mod_read_byte(PLAYER+0x14),psx_mod_read_byte(PLAYER+0x15)};
    put32(second.body+8,x);
    int32_t top=(int16_t)psx_mod_read_half(CAMERA+0x0E)-40;
    put32(second.body+12,(uint32_t)(top*65536));
    put32(second.body+0x18,le32(second.body+8));put32(second.body+0x1C,le32(second.body+12));
    /* Inventory survives a voluntary departure; its old jump/dash momentum
     * does not. This beam chooses a new grounded landing, so carrying a
     * negative saved Y velocity into idle would launch the returning hero. */
    put32(second.body+0x20,0);put32(second.body+0x24,0);
    second.body[3]=second.body[4]=1;second.body[5]=second.body[6]=0;
    enter_second();guest(cpu,0x8003CD08,PLAYER,0);
    join_y=(int32_t)foot_aligned_y(y,leader_box,psx_mod_read_word(PLAYER+0x68));
    guest(cpu,0x8003A2C8,PLAYER,0);guest(cpu,0x8003B33C,PLAYER,1);
    psx_mod_write_byte(PLAYER+0x14,facing[0]);psx_mod_write_byte(PLAYER+0x15,facing[1]);
    psx_mod_write_word(PLAYER+0x68,0);psx_mod_write_word(PLAYER+0x54,0);leave_second();
    life.status[1]=MMX5_COOP_JOINING;join_phase=1;
}
static void start_join(CPUState *cpu) {
    memcpy(second.body,returning_body,sizeof returning_body);
    begin_join(cpu,psx_mod_read_word(PLAYER+8),psx_mod_read_word(PLAYER+12));
}
static int body_on_support(unsigned seat,const uint8_t *body);
static int primary_grounded(void) {
    if(psx_mod_read_byte(PLAYER+0x70)&8)return 1;
    uint8_t body[0x158];capture(PLAYER,body,sizeof body);
    return body_on_support(0,body);
}
static void arrival_parts_event(CPUState *cpu) {
    /* Original35528 consumes this resolving-animation event before its
     * completion test. Zero's landing changes parts/animation directories;
     * leaving the event pending retains its two-frame beam flash. */
    if(!psx_mod_read_byte(PLAYER+0x45))return;
    psx_mod_write_byte(PLAYER+0x45,0);
    guest(cpu,0x8003D5E0u,PLAYER,0);guest(cpu,0x8003B65Cu,PLAYER,0);
}
static void stage_arrival_animation(CPUState *cpu,int advance) {
    uint8_t leader[0x158];capture(PLAYER,leader,sizeof leader);
    uint32_t x=le32(leader+8),y=le32(leader+12),box=le32(leader+0x68);
    int foot_owner=life.status[0]==MMX5_COOP_ALIVE && !leader[0xD5];
    int support=foot_owner && body_on_support(0,leader);
    int arriving_at_script=scene_owner==1 && join_phase>=3;
    int safe=foot_owner && leader[4]==1 && !leader[0x67] &&
        (!leader[0xD0] || arriving_at_script) && !leader[0xD4] &&
        (!leader[0x7A] || arriving_at_script) && box &&
        ((leader[0x70]&8) || support);
    enter_second();
    guest(cpu,0x8003CD08,PLAYER,0);
    if(join_phase==3) {
        psx_mod_write_byte(PLAYER+0x14,leader[0x14]);
        psx_mod_write_byte(PLAYER+0x15,leader[0x15]);
    }
    uint32_t landing=foot_aligned_y(y,box,psx_mod_read_word(PLAYER+0x68));
    uint32_t target=x;
    /* Once the beam has a legal landing, its resolving animation belongs
     * to that spot. A moving/jumping leader must not drag the passenger.
     * Re-anchor only if the original floor/platform has become unsafe. */
    int anchored=0;
    if(join_phase==4) {
        target=psx_mod_read_word(PLAYER+8);landing=(uint32_t)join_y;
        anchored=standing_return_position(cpu,&target,&landing,0,2);
    }
    if(anchored)safe=1;
    else if(safe) {
        target=x;landing=foot_aligned_y(y,box,psx_mod_read_word(PLAYER+0x68));
        safe=standing_return_position(cpu,&target,&landing,1,support);
    }
    /* Until the original leader finds a floor, mirror its one falling beam.
     * The incoming partner has no collision box or usable controls. */
    if(!safe) {
        target=x+(24u<<16);
        landing=join_phase==3?y:(uint32_t)join_y;
    }
    if(safe)join_y=(int32_t)landing;
    psx_mod_write_word(PLAYER+8,target);psx_mod_write_word(PLAYER+0x18,target);
    psx_mod_write_word(PLAYER+12,landing);psx_mod_write_word(PLAYER+0x1C,landing);
    psx_mod_write_word(PLAYER+0x68,0);
    if(advance)guest(cpu,0x80017248,PLAYER,0);
    if(join_phase==4)arrival_parts_event(cpu);
    if(join_phase==3 && safe) {
        guest(cpu,0x8003B33C,PLAYER,2);join_phase=4;
    } else if(join_phase==4 && safe && !psx_mod_read_byte(PLAYER+0x46)) {
        guest(cpu,0x8003A670,PLAYER,0);guest(cpu,0x8003CD08,PLAYER,0);
        guest(cpu,0x8002FB58,PLAYER,0);
        psx_mod_write_byte(PLAYER+0x61,0);psx_mod_write_byte(PLAYER+0xA4,0);
        join_phase=0;mmx5_coop_teleport_done(&life);
    }
    leave_second();
}
static void join_animation(CPUState *cpu,int advance) {
    if(join_phase>=3) {stage_arrival_animation(cpu,advance);return;}
    /* A native opening can move P1 while the incoming partner is suspended
     * (water-stage hull/camera admission). Its old target can then be inside
     * a solid. Keep the one native beam/resolving animation, but use the
     * current grounded leader's feet before enabling ordinary collision.
     * Mounted/chaser arrivals retain their separate native vehicle policy. */
    int foot_owner=life.status[0]==MMX5_COOP_ALIVE && !psx_mod_read_byte(PLAYER+0xD5);
    int safe=foot_owner && psx_mod_read_byte(PLAYER+4)==1 &&
        primary_grounded() && !psx_mod_read_byte(PLAYER+0x67) &&
        !psx_mod_read_byte(PLAYER+0xD0) && !psx_mod_read_byte(PLAYER+0xD4) &&
        !psx_mod_read_byte(PLAYER+0x7A) && psx_mod_read_word(PLAYER+0x68);
    uint32_t x=psx_mod_read_word(PLAYER+8),y=psx_mod_read_word(PLAYER+12);
    uint32_t leader_box=psx_mod_read_word(PLAYER+0x68);
    enter_second();
    int anchored=0;
    uint32_t target=psx_mod_read_word(PLAYER+8),landing=(uint32_t)join_y;
    /* The descending beam has already selected a landing too. Retaining
     * only phase2 still drags phase1 horizontally with a walking leader. */
    if((join_phase==1 || join_phase==2) && foot_owner) {
        guest(cpu,0x8003CD08,PLAYER,0);
        anchored=standing_return_position(cpu,&target,&landing,0,2);
        psx_mod_write_word(PLAYER+0x68,0);
        if(anchored) {
            psx_mod_write_word(PLAYER+8,target);psx_mod_write_word(PLAYER+0x18,target);
            join_y=(int32_t)landing;safe=1;
        }
    }
    if(safe && !anchored) {
        guest(cpu,0x8003CD08,PLAYER,0);
        join_y=(int32_t)foot_aligned_y(y,leader_box,psx_mod_read_word(PLAYER+0x68));
        psx_mod_write_word(PLAYER+0x68,0);
        psx_mod_write_word(PLAYER+8,x);psx_mod_write_word(PLAYER+0x18,x);
        if(join_phase==2 || (int32_t)psx_mod_read_word(PLAYER+12)>join_y) {
            psx_mod_write_word(PLAYER+12,(uint32_t)join_y);
            psx_mod_write_word(PLAYER+0x1C,(uint32_t)join_y);
        }
    }
    if(advance)guest(cpu,0x80017248,PLAYER,0);
    if(join_phase==2)arrival_parts_event(cpu);
    if(join_phase==1 && advance) {
        int32_t y=(int32_t)psx_mod_read_word(PLAYER+12)+8*65536;
        if(y>=join_y) {y=join_y;guest(cpu,0x8003B33C,PLAYER,2);join_phase=2;}
        psx_mod_write_word(PLAYER+12,(uint32_t)y);
    } else if(join_phase==2 && !psx_mod_read_byte(PLAYER+0x46) && (!foot_owner || safe)) {
        guest(cpu,0x8003A670,PLAYER,0);
        guest(cpu,0x8003CD08,PLAYER,0);
        if(safe)guest(cpu,0x8002FB58,PLAYER,0);
        /* Returning through a completed beam is not a new hurt event. */
        psx_mod_write_byte(PLAYER+0x61,0);psx_mod_write_byte(PLAYER+0xA4,0);
        join_phase=0;mmx5_coop_teleport_done(&life);
    }
    leave_second();
}
static void join_finish_after_world(CPUState *cpu) {
    /* Solid callbacks publish their current top contact after second_tick.
     * A resolving beam on a pad therefore cannot qualify the support at
     * that earlier body boundary. Commit after the real world contacts;
     * do not advance its native animation or world clocks a second time. */
    if(!busy && !projected && enrolled && !failed && !scene_owner &&
       psx_mod_read_byte(PLAY)==10 && !psx_mod_read_byte(PLAY+1) &&
       !psx_mod_read_byte(PLAY+0x10) && !psx_mod_read_byte(PLAY+0x1C) &&
       life.status[1]==MMX5_COOP_ABSENT) {
        /* The primary body clears solid contact before second_tick; only
         * the completed native solid pass can validate this fresh Select.
         * Consume absent-seat input here once, preserving its native edge. */
        int car=mmx5_coop_car_sequence(psx_mod_read_byte(PLAY+0x0C),psx_mod_read_byte(PLAY+0x1D),
            psx_mod_read_byte(0x800D1DC0) && !psx_mod_read_byte(0x800D1DC1));
        int gameplay=!car && !duel_scope() && !psx_mod_read_byte(PLAYER+0xD5) && !second.body[0xD5];
        int safe=!psx_mod_read_byte(PLAYER+0x67) && primary_grounded();
        if(mmx5_coop_join_input(&life,input(1)&0x100,gameplay,safe)==MMX5_COOP_JOIN)
            start_join(cpu);
    }
    if(!busy && !projected && !scene_owner && psx_mod_read_byte(PLAY)==10 &&
       !psx_mod_read_byte(PLAY+1) && !psx_mod_read_byte(PLAY+0x10) && !psx_mod_read_byte(PLAY+0x1C) &&
       life.status[1]==MMX5_COOP_JOINING &&
       (join_phase==3 || ((join_phase==2 || join_phase==4) && !second.body[0x46])))
        join_animation(cpu,0);
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
static uint32_t both_player_call(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1) {
    /* Duel losers animate once at the canonical world boundary. Their
     * zero-HP crouch must never dispatch ordinary fatal/death-orb logic. */
    if(address==0x80035040u && duel_loser_body())return 0;
    /* Native352A8 folds D4 into D3, disabling ordinary controls even for a
     * negative gate. Only the body pass may ignore our pending crossing;
     * world/boss scripts must continue seeing the gate until both clear. */
    unsigned gate=psx_mod_read_byte(PLAYER+0xD4);
    int scoped=both_door_scope() && both_door.phase==2 && !both_door_native_carry() && gate==255 &&
        !psx_mod_local_view_scope();
    if(scoped)psx_mod_write_byte(PLAYER+0xD4,0);
    uint32_t result=guest(cpu,address,a0,a1);
    if(scoped)psx_mod_write_byte(PLAYER+0xD4,(uint8_t)gate);
    return result;
}
static void second_tick(CPUState *cpu,uint32_t address) {
    (void)address;
    if(busy || projected || failed || cpu->gpr[31]!=0x80022DC8u)return;
    if(psx_mod_read_byte(PLAY)!=10 || !psx_mod_read_byte(PLAYER)) {enrolled=0;return;}
    /* The native car arrival drives its rider while PLAY+1C is set. Enroll
     * and advance the second car during that arrival too, so it shares the
     * authored opening instead of joining after the first drop. */
    int incoming=!car_context() && mmx5_coop_native_incoming(psx_mod_read_byte(PLAYER+4),
        psx_mod_read_byte(PLAYER+5),psx_mod_read_byte(PLAYER+0x67),
        psx_mod_read_byte(PLAYER+0xD5),scene_owner || psx_mod_read_byte(PLAYER+0xD0) ||
        psx_mod_read_byte(PLAYER+0xD4) || psx_mod_read_byte(PLAYER+0x7A));
    /* Ground contact clears67 before the primary's resolving animation
     * finishes. Its remaining stage hold must not suspend the other native
     * arrival animation; ordinary input waits in both_player_call above. */
    int pair_arrival=enrolled && !car_context() && !scene_owner &&
        life.status[1]==MMX5_COOP_ALIVE && psx_mod_read_byte(PLAYER+4)==1 &&
        !psx_mod_read_byte(PLAYER+5) && !psx_mod_read_byte(PLAYER+0xD5) &&
        !psx_mod_read_byte(PLAYER+0xD0) && !psx_mod_read_byte(PLAYER+0xD4) &&
        !psx_mod_read_byte(PLAYER+0x7A);
    /* The native script flag also covers a door's closing animation and
     * boss presentation after its player hold has ended. The ordinary P1
     * pass already accepts input then; advance P2 under the same released
     * body contract, including contacts and damage, rather than waiting for
     * presentation to finish. Active shared scenes still own their bodies. */
    int pair_controls=enrolled && !car_context() && !scene_owner &&
        life.status[0]==MMX5_COOP_ALIVE && life.status[1]==MMX5_COOP_ALIVE &&
        psx_mod_read_byte(PLAYER+4)==1 && psx_mod_read_byte(PLAYER+5)>=2 &&
        !psx_mod_read_byte(PLAYER+0xD0) && !psx_mod_read_byte(PLAYER+0xD4) &&
        !psx_mod_read_byte(PLAYER+0x7A);
    int door_return=both_door_native_carry() && both_door.phase==4 &&
        !scene_owner && second.body[4]==1 && second.body[5]==0x13;
    if(psx_mod_read_byte(PLAY+0x10) ||
       (!car_context() && (scene_owner || psx_mod_read_byte(PLAY+0x1C)) &&
        !incoming && !pair_arrival && !pair_controls && !personal_giga() && !door_return &&
        !(enrolled && join_phase>=3 && scene_owner<=1) &&
        !(enrolled && life.wipe && life.status[1]==MMX5_COOP_DYING))) {life.select_frames=0;return;}
    if(!enrolled && (psx_mod_read_byte(PLAYER+4)!=1 ||
       (psx_mod_read_byte(PLAYER+5)!=2 && !incoming &&
        !(car_context() && psx_mod_read_byte(PLAYER+5)==0x12))))return;
    /* 3D784 activates the original slot during the player pass, AFTER that
     * frame's vehicle dispatcher. Wait for its next native initialization
     * before copying a car; pose12 alone can still have a zero vehicle body. */
    if(!enrolled && car_context() && psx_mod_read_byte(PLAYER+5)==0x12 &&
       (!psx_mod_read_byte(CAR) || psx_mod_read_byte(CAR+1) || psx_mod_read_byte(CAR+4)!=1))return;
    busy=1;
    if(!ready) {ready=load_assets(1);if(!ready)failed=1;}
    if(ready && !enrolled && !enroll(cpu))failed=1;
    if(enrolled && !failed && life.status[1]!=MMX5_COOP_ABSENT) {
        uint16_t raw=input(1);
        /* Native terrain and a currently contacted solid are both legal
         * return anchors. Glow's sender/capsule tops use solid contact71,
         * not terrain contact70; join_animation already validates them. */
        int safe=!psx_mod_read_byte(PLAYER+0x67) && primary_grounded();
        int car=mmx5_coop_car_sequence(psx_mod_read_byte(PLAY+0x0C),psx_mod_read_byte(PLAY+0x1D),
            psx_mod_read_byte(0x800D1DC0) && !psx_mod_read_byte(0x800D1DC1));
        if(car && (raw&0x100))psx_mod_counter_add("mmx5.coop.car-departure-blocked",1);
        int gameplay=!car && !duel_scope() && !psx_mod_read_byte(PLAYER+0xD5) && !second.body[0xD5];
        Mmx5CoopJoinAction action=mmx5_coop_join_input(&life,raw&0x100,gameplay,safe);
        if(action==MMX5_COOP_LEAVE)start_leave(cpu);
        else if(action==MMX5_COOP_JOIN)start_join(cpu);
    }
    if(enrolled && !failed && life.status[1]==MMX5_COOP_JOINING) {
        join_animation(cpu,1);++ticks;diagnostics();busy=0;return;
    }
    if(enrolled && !failed && life.status[1]!=MMX5_COOP_FALLEN && life.status[1]!=MMX5_COOP_ABSENT &&
       life.status[1]!=MMX5_COOP_UNAVAILABLE) {
        uint8_t saved_input[6],origins[3][20],camera_follow[3];capture(PAD,saved_input,sizeof saved_input);
        int own_camera=split_views && split_valid && !scene_owner && !car_context();
        int arriving_camera=own_camera && second.body[4]<=1 && second.body[5]<2;
        if(own_camera)for(unsigned layer=0;layer<3;++layer) {
            unsigned follow=layer*0x54u+0x44u;
            camera_follow[layer]=psx_mod_read_byte(CAMERA+follow);
            psx_mod_write_byte(CAMERA+follow,split_cameras[1][follow]);
            capture(CAMERA+layer*0x54u+8,origins[layer],20);
            project(CAMERA+layer*0x54u+8,split_cameras[1]+layer*0x54u+8,20);
        }
        uint16_t raw=input(1);enter_second();partner_giga_control();
        /* Native body gates own door carry and introduction holds. Once
         * released, accept input during closure just as the primary does. */
        /* 3CDB8 caches input before35040. Each incoming body's native
         * program owns its own control release: Zero can finish before X.
         * Shared scenes and scripted door carries retain their native gates. */
        int held=life.status[1]==MMX5_COOP_LEAVING;
        psx_mod_write_half(PAD,held?0:raw);psx_mod_write_half(PAD+2,held?0:previous_input);
        psx_mod_write_half(PAD+4,held?0:(uint16_t)(raw&~previous_input));previous_input=raw;
        guest(cpu,0x8003CDB8,0,0);
        if(car_context()) {
            uint8_t other=psx_mod_read_byte(CAR+0xB0);psx_mod_write_byte(CAR+0xB0,0);
            guest(cpu,0x80022960,0,0);psx_mod_write_byte(CAR+0xB0,other);
        } else ride_second_tick(cpu);
        both_player_call(cpu,0x80035040,0,0);
        guest(cpu,0x80022084,0,0);
        if(life.status[1]!=MMX5_COOP_ALIVE || psx_mod_read_byte(PLAYER+4)!=1)clear_personal_drawables(1);
        if(own_camera) {
            /* The singleton camera starts disabled during native arrival.
             * Each hero completes its own incoming program; P2 must enable
             * its view then, without waiting for P1's longer animation.
             * A later native fatal disables only this private view. */
            if(arriving_camera && psx_mod_read_byte(PLAYER+4)==1 &&
               psx_mod_read_byte(PLAYER+5)>=2 && !psx_mod_read_byte(PLAYER+0xD0) &&
               !psx_mod_read_byte(PLAYER+0xD4))psx_mod_write_byte(CAMERA+0x44,1);
            /* Native354F0 and3C394 enable all three layer follows; the
             * outgoing program likewise clears all three. Keep those native
             * writes with this view before leave_second restores its caller.
             * Losing the secondary enables pins their origins to spawn-time
             * tiles even while the foreground camera follows the hero. */
            for(unsigned layer=0;layer<3;++layer) {
                unsigned follow=layer*0x54u+0x44u;
                split_cameras[1][follow]=psx_mod_read_byte(CAMERA+follow);
            }
        }
        second_terrain_pending=1;
        leave_second();project(PAD,saved_input,sizeof saved_input);++ticks;
        if(own_camera)for(unsigned layer=0;layer<3;++layer) {
            project(CAMERA+layer*0x54u+8,origins[layer],20);
            psx_mod_write_byte(CAMERA+layer*0x54u+0x44u,camera_follow[layer]);
        }
        if(life.status[1]==MMX5_COOP_LEAVING && second.body[4]==3)mmx5_coop_teleport_done(&life);
        if(second.body[4]==2) {clear_buster_effects(1);mmx5_coop_fatal(&life,1);}
        if(life.status[1]==MMX5_COOP_DYING && second.body[4]==3)life.status[1]=MMX5_COOP_FALLEN;
        psx_mod_counter_add("mmx5.coop.player-ticks",1);
    }
    diagnostics();busy=0;
}
static int player_terrain(CPUState *cpu,uint32_t address) {
    if(busy || projected || psx_mod_local_view_scope() ||
       cpu->gpr[31]!=0x80022E20u || cpu->gpr[4]!=PLAYER)return 0;
    if(second_terrain_pending) {
        second_terrain_pending=0;
        if(enrolled && !failed) {
            /* Native22D98 resolves terrain after the shared solid pass.
             * Solid3240C can itself call2FB58 while moving a hero. An early
             * P2 terrain pass lets that later call erase uphill contact,
             * alternating Zero's walk/fall pose. Resolve P2 at the same
             * boundary as P1, then leave P1's final collision scratch native. */
            busy=1;enter_second();guest(cpu,address,PLAYER,0);leave_second();busy=0;
            psx_mod_counter_add("mmx5.coop.p2-final-terrain-passes",1);
        }
    }
    return 0;
}
static int player_tick(CPUState *cpu,uint32_t address) {
    if(player_call || busy || !enrolled || failed)return 0;
    if(projected) {
        if(!world_owner)return 0;
        /* SCRATCH+28 is the entire native stage palette, not just the rider's
         * colors. Keep P2's color changes private during its body pass. */
        uint32_t palette=psx_mod_read_word(SCRATCH+0x28);
        psx_mod_write_word(SCRATCH+0x28,assets[counterpart].palette);
        player_call=1;uint32_t result=both_player_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);player_call=0;
        if(psx_mod_read_byte(PLAYER+4)==2) {clear_buster_effects(1);clear_personal_drawables(1);}
        psx_mod_write_word(SCRATCH+0x28,palette);cpu->gpr[2]=result;return 1;
    }
    psx_mod_counter_add("mmx5.coop.primary-player-calls",1);
    uint8_t freeze[9];capture(PLAY+0x12,freeze,sizeof freeze);
    uint8_t script_lock=psx_mod_read_byte(PLAY+0x1C);
    player_call=1;uint32_t result=both_player_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);player_call=0;
    if(psx_mod_read_byte(PLAYER+4)==2 || life.status[0]==MMX5_COOP_DYING) {
        psx_mod_counter_add("mmx5.coop.primary-death-ticks",1);
        clear_buster_effects(0);
        clear_personal_drawables(0);
        mmx5_coop_fatal(&life,0);
        if(mmx5_coop_living(&life,1)) {
            project(PLAY+0x12,freeze,sizeof freeze);
            /* Native38C08 locks the single-player pause menu on fatal entry.
             * The living partner retains the pre-death scene lock instead;
             * an already-held native script must still finish normally. */
            psx_mod_write_byte(PLAY+0x1C,script_lock);
        }
        if(psx_mod_read_byte(PLAYER+4)==3)life.status[0]=MMX5_COOP_FALLEN;
    }
    cpu->gpr[2]=result;return 1;
}
static void fatal_pause_lock(CPUState *cpu,uint32_t address) {
    uint32_t instruction=psx_mod_read_word(address);
    unsigned base=(instruction>>21)&31u,reg=(instruction>>16)&31u;
    if(!enrolled || failed || psx_mod_local_view_scope() || duel_scope() ||
       psx_mod_read_byte(PLAY)!=10 || cpu->gpr[16]!=PLAYER ||
       cpu->gpr[base]+(int16_t)instruction!=PLAY+0x1Cu || cpu->gpr[reg]!=1)return;
    unsigned seat=psx_mod_read_byte(PLAYER+2)==counterpart;
    if(life.status[seat^1u]!=MMX5_COOP_ALIVE)return;
    /* Fatal contact/grab paths mark HP at318F4/31B50/3D088;38C08 then
     * initializes death. Each source register is replaced before reuse.
     * Their single-player menu lock otherwise survives into the next pass,
     * suppressing private presentation and the living partner's pause.
     * Keep the pre-fatal shared lock, including an already active script.
     * Death, native effects and the survivor's world still run normally. */
    cpu->gpr[reg]=psx_mod_read_byte(PLAY+0x1C);
    psx_mod_counter_add("mmx5.coop.fatal-pause-locks-retained",1);
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
static int personal_visual_tick(CPUState *cpu,uint32_t address) {
    uint32_t caller=address==0x80022EDCu?0x80022E50u:0x80022E58u;
    if(personal_visual_call || busy || projected || !enrolled || failed ||
       psx_mod_local_view_scope() || psx_mod_read_byte(PLAY)!=10 || cpu->gpr[31]!=caller)return 0;
    /* The original world advances these pools after contacts and camera
     * movement. Keep that boundary/order and run only the matching personal
     * update for P2; the surrounding world and its timers remain once-only. */
    personal_visual_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    if(life.status[1]==MMX5_COOP_ALIVE && second.body[4]==1) {
        enter_second();guest(cpu,address,0,0);leave_second();
        ++personal_visual_ticks[address==0x80022EDCu?0:1];
        psx_mod_counter_add(address==0x80022EDCu?"mmx5.coop.p2-charge-glow-ticks":
                                               "mmx5.coop.p2-dash-trail-ticks",1);
    } else clear_personal_drawables(1);
    personal_visual_call=0;cpu->gpr[2]=result;return 1;
}
#include "mmx5_coop_personal_menu.inc"
static int stage_tick(CPUState *cpu,uint32_t address) {
    if(stage_call || busy || projected || !enrolled || failed)return 0;
    psx_mod_counter_add("mmx5.coop.stage-dispatch-hooks",1);
    both_door_maintain(cpu);
    scene_recover_owner(cpu);
    unsigned phase=psx_mod_read_byte(PLAY+1);
    uint16_t raw=input(1);int start=!!(raw&0x0800),pressed=start&&!p2_start_previous;
    p2_start_previous=start;
    if(scene_owner && !phase && !psx_mod_read_byte(PLAY+12) &&
       !psx_mod_read_byte(PLAY+13) && psx_mod_read_word(0x800EE970u)==4) {
        /* A shared intro suspends the passenger's ordinary attack pass.
         * Letting go must still cancel its retained charge, using the native
         * cleanup that stops the charge sound and clears9B..9E. Neither
         * advance a held charge nor queue a shot inside the scripted pose. */
        for(unsigned seat=0;seat<2;++seat) {
            const uint8_t *body=seat?second.body:first.body;
            uint16_t held=seat?raw:psx_mod_read_half(PAD);
            if(life.status[seat]!=MMX5_COOP_ALIVE || body[2] || body[4]!=1 ||
               !body[0xD0] || (held&0x0090u) ||
               !(body[0x9B]|body[0x9C]|body[0x9D]) && body[0x9E]<=1)continue;
            if(seat)enter_second();
            guest(cpu,0x8003F250,PLAYER,0);
            /* Native attack release3F35C also restores the selected armor's
             * palette. The script suspends that ordinary attack pass, so
             * cancelling only its charge leaves the last flash colors held.
             * Timer9E==1 is the native idle increment, not a new charge. */
            guest(cpu,0x8003C5D8,PLAYER,0);
            clear_personal_drawables(seat);
            if(seat)leave_second();
        }
    }
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
    if(phase==2 && !personal_active) {
        personal_menu_begin();
        if(!menu_active)menu_vram(cpu,1);
    }
    int survivor=life.wipe==2 || (!life.wipe &&
        (life.status[0]==MMX5_COOP_DYING || life.status[0]==MMX5_COOP_FALLEN) &&
        mmx5_coop_living(&life,1));
    if(personal_active || (!survivor && pause_owner!=1 && scene_owner!=2)) {
        stage_call=1;uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        ride_deferred_slots=0;
        if(car_context()) {capture_player(&first);cars_refresh(0);}
        enforce_roster();duel_after_world(cpu);
        if(!psx_mod_read_byte(PLAY+1))scene_tick(cpu);
        if(menu_active && psx_mod_read_byte(PLAY+1)!=2)menu_vram(cpu,0);
        join_finish_after_world(cpu);
        diagnostics();if(psx_mod_read_byte(PLAY)!=10)enrolled=0;
        stage_call=0;cpu->gpr[2]=result;return 1;
    }
    stage_call=1;
    if(pause_owner==1 && !menu_active)menu_vram(cpu,1);
    if(survivor)retain_survivor(cpu);
    /* Advance only P1's death pose before projecting the surviving actor for
     * the one shared native stage pass. Its freeze/timer side effects belong
     * to the canonical owner. Finish this pose even if P2 has since died and
     * owns the retry; otherwise P1's animation stops at the second fatal. */
    if(psx_mod_read_byte(PLAY+1)==0 && psx_mod_read_byte(PLAYER+4)==2) {
        uint8_t freeze[9];capture(PLAY+0x12,freeze,sizeof freeze);
        uint32_t timer=psx_mod_read_word(PLAY+0x1B4),countdown=psx_mod_read_word(PLAY+0xAC);
        if(car_context())guest(cpu,0x80022960,0,0);
        player_call=1;guest(cpu,0x80035040,0,0);player_call=0;
        project(PLAY+0x12,freeze,sizeof freeze);
        psx_mod_write_word(PLAY+0x1B4,timer);psx_mod_write_word(PLAY+0xAC,countdown);
        if(psx_mod_read_byte(PLAYER+4)==3)life.status[0]=MMX5_COOP_FALLEN;
    }
    uint8_t saved_input[6];capture(PAD,saved_input,sizeof saved_input);
    uint16_t story_confirm=0,story_confirm_edge=0;
    uint32_t text_cursor=psx_mod_read_word(0x80091C38u);
    uint32_t personal_root=assets[counterpart].message_root;
    int personal_text=personal_root && text_cursor>=personal_root &&
        text_cursor-personal_root<assets[counterpart].message_bytes;
    if(scene_owner==2 && !phase && life.status[0]==MMX5_COOP_ALIVE && !personal_text &&
       psx_mod_read_byte(0x80091C44u)) {
        /* Any shared stage scene can belong to the P2 entrant, including
         * Maverick introductions. P1's freshly sampled confirm still
         * advances that scene's native dialogue, as it does for Sigma.
         * Forward confirm only while native text is actually active. During
         * a room or gravity transport the same button is gameplay input and
         * must not let the passenger make the scene owner jump.
         * Capsule pages use their recipient's private message bank and keep
         * that recipient's input. Retain the native edge: a button held before the scene must not
         * become a new press when the stage projects the other hero. */
        story_confirm=psx_mod_read_half(PAD)&0x0040u;
        story_confirm_edge=psx_mod_read_half(PAD+4)&0x0040u;
    }
    enter_second();
    /* Native story and stage selection remain P1's. Menus alone select the
     * requesting actor's character identity. Player routines use PLAYER+2. */
    if(pause_owner!=1)psx_mod_write_byte(PLAY+0x44,duel_scope()?
        story_duel.character[story_duel.entrant]:saved_play[0x44]);
    /* Original 56514 caches stage palette pointers in world actors; 2BD8C
     * builds the shared fade from the whole 4000-byte buffer, and 17484
     * uploads it. P2's character-only palette must never own those passes.
     * Body/attack filters above temporarily select its private colors. */
    if(pause_owner!=1)psx_mod_write_word(SCRATCH+0x28,resources[3]);
    psx_mod_write_half(PAD,(uint16_t)(raw|story_confirm));
    psx_mod_write_half(PAD+2,(uint16_t)(previous_input|(story_confirm&~story_confirm_edge)));
    psx_mod_write_half(PAD+4,(uint16_t)((raw&~previous_input)|story_confirm_edge));previous_input=raw;
    world_owner=1;world_packets=arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_BYTES;
    uint32_t world_before=world_ticks;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    ride_deferred_slots=0;
    if(psx_mod_read_byte(PLAYER+4)==2) {clear_buster_effects(1);mmx5_coop_fatal(&life,1);}
    if(life.status[1]==MMX5_COOP_DYING && psx_mod_read_byte(PLAYER+4)==3)life.status[1]=MMX5_COOP_FALLEN;
    world_owner=0;leave_second_context(1);project(PAD,saved_input,sizeof saved_input);
    enforce_roster();duel_after_world(cpu);
    if(!psx_mod_read_byte(PLAY+1))scene_tick(cpu);
    join_finish_after_world(cpu);
    ticks+=world_ticks-world_before;psx_mod_counter_add("mmx5.coop.survivor-world-ticks",world_ticks-world_before);diagnostics();
    if(menu_active && psx_mod_read_byte(PLAY+1)!=2)menu_vram(cpu,0);
    if(psx_mod_read_byte(PLAY)!=10)enrolled=0;
    stage_call=0;cpu->gpr[2]=result;return 1;
}
static DeathOrigin *death_origin(uint32_t actor) {
    if(actor<0x800D1F40u || actor>=0x800D4F40u || (actor-0x800D1F40u)%0x60)return NULL;
    return &death_origins[(actor-0x800D1F40u)/0x60];
}
static int private_death_actor(uint32_t actor) {
    if(!death_origin(actor) || psx_mod_read_byte(actor+1)!=0x11 ||
       !(psx_mod_read_byte(actor)&0x20u) || psx_mod_read_word(actor+0x38))return 0;
    uint32_t assembly=assets[counterpart].assembly;
    return assembly && psx_mod_read_word(actor+0x3C)==assembly+psx_mod_read_word(assembly+4);
}
static LandingEffect *landing_effect(uint32_t actor) {
    if(actor<0x800D1F40u || actor>=0x800D4F40u || (actor-0x800D1F40u)%0x60)return NULL;
    return &landing_effects[(actor-0x800D1F40u)/0x60];
}
static int water_head_stage(void) {
    return psx_mod_read_byte(PLAY)==10 && psx_mod_read_byte(PLAY+12)==3 &&
        !psx_mod_read_byte(PLAY+13) && psx_mod_read_word(0x800EE970u)==8 &&
        psx_mod_read_word(0x800EECA8u)==0x0C00B7DEu &&
        psx_mod_read_word(0x800F7F4Cu)==0x2463FFEEu &&
        psx_mod_read_word(0x800F7F5Cu)==0x14400004u &&
        psx_mod_read_word(0x800F7F68u)==0x0803DFDFu &&
        psx_mod_read_word(0x800F7F78u)==0x2442000Cu;
}
static int camera_emitter_stage(void) {
    return psx_mod_read_byte(PLAY)==10 && psx_mod_read_byte(PLAY+12)==4 &&
        !psx_mod_read_byte(PLAY+13) && psx_mod_read_word(0x800EE970u)==9 &&
        psx_mod_read_word(0x800F48E4u)==0x800F36F8u &&
        psx_mod_read_word(0x800F4C90u)==0x800F3744u &&
        psx_mod_read_word(0x800F5888u)==0x800F2798u &&
        psx_mod_read_word(0x800F58ECu)==0x800F3C44u;
}
static int camera_emitter_parent(uint32_t actor) {
    return actor>=0x80092090u && actor<0x80093DD0u && (actor-0x80092090u)%0x9Cu==0 &&
        psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==0x25 &&
        !psx_mod_read_byte(actor+2) && psx_mod_read_byte(actor+4)==1;
}
static uint16_t camera_emitter_view(unsigned seat,unsigned at) {
    return (uint16_t)(split_cameras[seat][at]|split_cameras[seat][at+1]<<8);
}
static int effect_allocate(CPUState *cpu,uint32_t address) {
    if(effect_allocate_call || !enrolled || failed)return 0;
    uint32_t caller=cpu->gpr[31];
    uint32_t sprites=psx_mod_read_word(SCRATCH+0x14),assembly=psx_mod_read_word(SCRATCH+0x1C);
    unsigned zero=psx_mod_read_byte(PLAYER+2)==1;
    /* Ride Armor may temporarily restore P1 inside an outer projected scope. */
    unsigned owner=projected && psx_mod_read_byte(PLAYER+2)==counterpart?2u:1u;
    effect_allocate_call=1;
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
    LandingEffect *landing=landing_effect(actor);
    if(landing && !psx_mod_local_view_scope()) {
        memset(landing,0,sizeof *landing);
        /* The caller assigns live/class3F/sub0 only after the allocator
         * returns. Match its exact RA now; verify the class at use below. */
        if(caller==0x80043390u && zero) {
            landing->sprites=sprites;landing->assembly=assembly;landing->owner=(uint8_t)owner;
        }else if(caller==0x800EEC88u && water_head_stage()) {
            /* The native stage timer admits this head now; its deferred
             * initializer must still read the same logical hero later. */
            landing->owner=(uint8_t)owner;
        }else if((caller==0x800F3CD8u || caller==0x800F3D84u) && ai_call &&
                 camera_emitter_stage() && camera_emitter_parent(cpu->gpr[16])) {
            /* Native class25 assigns class14/sub3 or4 and parent50 only
             * after this allocation. Keep its logical seat before deferred
             * initialization reads a camera with no coordinates yet. */
            landing->owner=(uint8_t)owner;
            if(owner==2)psx_mod_counter_add("mmx5.coop.p2-camera-emitter-allocations",1);
        }
    }
    cpu->gpr[2]=actor;return 1;
}
static int landing_effect_tick(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];LandingEffect *landing=landing_effect(actor);
    if(effect_init_call || !enrolled || failed || psx_mod_local_view_scope() ||
       !landing || !landing->owner || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=0x3F || psx_mod_read_byte(actor+2))return 0;
    unsigned initialize=!psx_mod_read_byte(actor+4);
    uint32_t sprites=psx_mod_read_word(SCRATCH+0x14),assembly=psx_mod_read_word(SCRATCH+0x1C);
    if(initialize) {
        /* Original4C620 captures both directories and performs its native
         * upload. Its later animation/spark/retirement calls run unchanged. */
        psx_mod_write_word(SCRATCH+0x14,landing->sprites);
        psx_mod_write_word(SCRATCH+0x1C,landing->assembly);
    }
    effect_init_call=1;uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);effect_init_call=0;
    if(initialize) {
        psx_mod_write_word(SCRATCH+0x14,sprites);psx_mod_write_word(SCRATCH+0x1C,assembly);
    }
    if(!psx_mod_read_byte(actor))memset(landing,0,sizeof *landing);
    cpu->gpr[2]=result;return 1;
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
    balance_forget(actor);
    int source=car_source_index(actor);
    if(source>=0)car_source_owner[source]=(uint8_t)(car_context()?(projected?2:1):0);
    HitHistory *h=hit_record(actor,0);
    if(h) {unsigned i=(unsigned)(h-hit_history);--hit_count;
        memmove(h,h+1,(hit_count-i)*sizeof *h);memset(hit_history+hit_count,0,sizeof *h);}
    SolidHistory *s=solid_record(actor);if(s)memset(s,0,sizeof *s);
    for(unsigned i=0;i<128;++i)if(pickup_owners[i].actor==actor)pickup_owners[i].actor=0;
}
static int allocate_actor(CPUState *cpu,uint32_t address) {
    /* Native Gaea construction queues type1B graphics effects before P2's
     * enrollment completes. They already belong to the projected body;
     * deferring their owner tag makes50D04 decode Zero's packed art as X's
     * weapon directory and overwrite guest code. */
    if(allocation_call || (!enrolled && !projected) || failed)return 0;
    uint32_t stack=cpu->gpr[29];
    int pegasus_burst=address==0x8002D0E4u && cpu->gpr[31]==0x8004FFC0u &&
        !(stack&3u) && stack>=0x80010000u && stack<=0x801FFFE0u &&
        psx_mod_read_word(stack+0x1C)==0x800EF6E0u &&
        psx_mod_read_byte(PLAY+12)==8 && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==15u &&
        psx_mod_read_word(0x800EF6D8u)==0x0C013FE5u;
    uint32_t burst_x=pegasus_burst?psx_mod_read_word(CAMERA+8):0;
    uint32_t burst_y=pegasus_burst?psx_mod_read_word(CAMERA+12):0;
    uint32_t emitter=pegasus_burst?psx_mod_read_word(stack+0x18):0;
    uint32_t blast_origin=0;
    if(emitter>=0x8009E7A0u && emitter<0x8009F9A0u &&
       !((emitter-0x8009E7A0u)%0x30u) &&
       psx_mod_read_word(emitter+0x20)==0x800EF308u)
        blast_origin=psx_mod_read_half(emitter+0x26)|
            (uint32_t)psx_mod_read_half(emitter+0x2A)<<16;
    uint32_t bomb=cpu->gpr[16];
    int pegasus_sequence=address==0x8002D144u && cpu->gpr[31]==0x800EF31Cu &&
        psx_mod_read_byte(PLAY+12)==8 && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==15u &&
        psx_mod_read_word(0x800EF314u)==0x0C00B451u &&
        bomb>=0x80092090u && bomb<0x80093DD0u &&
        !((bomb-0x80092090u)%0x9Cu) &&
        psx_mod_read_byte(bomb) && psx_mod_read_byte(bomb+1)==0x1F;
    uint32_t bomb_x=pegasus_sequence?psx_mod_read_word(bomb+8):0;
    uint32_t bomb_y=pegasus_sequence?psx_mod_read_word(bomb+12):0;
    allocation_call=1;uint32_t actor=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);allocation_call=0;
    if(actor)forget_actor(actor);
    if(pegasus_sequence && actor>=0x8009E7A0u && actor<0x8009F9A0u &&
       !((actor-0x8009E7A0u)%0x30u)) {
        /* EF308 creates the native area5 presentation controller before the
         * physical blast. Its original program uses18..1D, leaving this tail
         * clear. Keep the actual bomb origin after the blast actor retires;
         * the controller's camera-relative clouds must not acquire distant
         * panes merely because those panes also remain active. */
        psx_mod_write_word(actor+0x20,0x800EF308u);
        psx_mod_write_word(actor+0x24,bomb_x);
        psx_mod_write_word(actor+0x28,bomb_y);
    }
    /* Original2D0E4 clears one70h shared-effect slot. A new P1/world
     * allocation must also clear a previous P2 owner of that same slot. */
    if(address==0x8002D0E4u && actor>=0x80094398u && actor<0x80096D98u &&
       !((actor-0x80094398u)%0x70u)) {
        personal_effect_owner[(actor-0x80094398u)/0x70u]=(uint8_t)(projected!=0);
        if(pegasus_burst) {
            /* EF5A0 emits the post-bomb cloud relative to CAMERA through
             *4FF94. Class4's native program leaves this cleared tail unused.
             * Retain its reference origin in guest state so each private
             * draw can reuse the one native animation/RNG/lifetime. The
             * ordinary explosion at the bomb does not take this call path. */
            psx_mod_write_word(actor+0x60,0x800EF5A0u);
            psx_mod_write_word(actor+0x64,burst_x);
            psx_mod_write_word(actor+0x68,burst_y);
            psx_mod_write_word(actor+0x6C,blast_origin);
        }
    }
    cpu->gpr[2]=actor;return 1;
}
static int personal_deferred_effect_canonical(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(personal_effect_call || projected || !enrolled || failed ||
       psx_mod_local_view_scope() || actor<0x80094398u || actor>=0x80096D98u ||
       (actor-0x80094398u)%0x70u)return 0;
    unsigned type=psx_mod_read_byte(actor+1);
    if(!((type==0 && address==0x8004F38Cu) ||
         (type==1 && address==0x8004F0B4u) ||
         (type==2 && address==0x8004F3F4u) ||
         (type==3 && address==0x8004F8E8u) ||
         (type==5 && address==0x80046900u) ||
         (type==7 && address==0x800EF0A0u && rose_stage_program()) ||
         (type==0x11 && address==0x8004C4DCu) ||
         (type==0x1B && address==0x80050D04u) ||
         (type==0x21 && address==0x80050F58u)))return 0;
    if(type==5) {
        uint32_t parent=psx_mod_read_word(actor+0x50);
        if(parent<SHOTS || parent>=SHOTS+sizeof second.shots || (parent-SHOTS)%0x9Cu)return 0;
    }
    unsigned owner=personal_effect_owner[(actor-0x80094398u)/0x70u];
    /* Type2 defers X's release through4F3F4 ->4F1FC until the shared
     * effect pass. It reads PLAYER/assembly and starts group1 SFX at4F348.
     * Preserve the allocating seat for the whole original effect update,
     * including any native shot children, then restore the shared context.
     * Type1 ordinary dash exhaust similarly reads PLAYER/assembly at4F0B4
     * and follows that body through4F654 on every update. Its native expiry
     * tests the same body's animation, so capturing an origin alone is not
     * enough. Type5 is C-Shot's attached effect: native46900 follows the
     * allocating shot at +50 and checks its owner body's cancellation pose.
     * Its numeric SHOTS pointer must resolve in that seat's private pool.
     * Type1B uploads X weapon graphics later through50D04/17348. Its native
     * directory indices require the allocating X bank: Zero's +18 is packed
     * frame data, so treating it as weapon5's offset makes182D8 overrun PLAY.
     * Rose's type7 surface ripples also read PLAYER on every update, including
     * native water contact and retirement. Use their allocating hero rather
     * than moving both sets of ripples onto the canonical body.
     * Type21 is Fourth Armor's hover exhaust. Native50F58 derives its pose
     * and position from PLAYER every tick and retires when that body's frame
     * leaves E5/E6/DD..E4. Testing P1 Zero deletes P2's plume immediately.
     * Type3 includes Zero's Z-Buster muzzle (subE) and wall-jump dust.
     * Native4F8E8 initializes their character art and follows PLAYER; subE
     * also checks that body's action3E before retiring. Keep the allocator
     * through the entire update, not just the initial coordinates.
     * Type0 is the separate wall-slide dust program. Its4F5CC helper reads
     * the current body both at initialization and throughout the slide;
     * retaining the type3 jump puff alone leaves this dust following P1.
     * Type11 is Zero's C-Sword: 4C4DC retires unless PLAYER is in animation60,
     * then obtains that character's art and follows its X/Y. Keep the same
     * allocating body through its native admission, upload and lifetime.
     * Leave unrelated shared effects on their original paths. */
    personal_effect_call=1;if(owner)enter_second();
    unsigned shots_before=psx_mod_read_byte(PLAYER+0x98);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    if(type==2 && psx_mod_read_byte(PLAYER+0x98)==((shots_before+1)&255u))
        psx_mod_counter_add(owner?"mmx5.coop.p2-buster-child-allocations":
                                 "mmx5.coop.p1-buster-child-allocations",1);
    if(owner)leave_second();personal_effect_call=0;
    if(owner)psx_mod_counter_add(type==2?"mmx5.coop.p2-buster-effect-ticks":
                               type==3 || type==0x11?"mmx5.coop.p2-body-effect-ticks":
                               type==5?"mmx5.coop.p2-cshot-effect-ticks":
                               type==7?"mmx5.coop.p2-water-ripple-ticks":
                               type==0x1B?"mmx5.coop.p2-weapon-graphics-effect-ticks":
                               type==0x21?"mmx5.coop.p2-hover-exhaust-ticks":
                                       "mmx5.coop.p2-dash-exhaust-ticks",1);
    cpu->gpr[2]=result;return 1;
}
static int personal_deferred_effect(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(projected && world_owner && !ai_suspended && !personal_effect_call &&
       enrolled && !failed && !psx_mod_local_view_scope() &&
       actor>=0x80094398u && actor<0x80096D98u && !((actor-0x80094398u)%0x70u)) {
        /* P2-owned scenes still update P1's queued personal effects. Resolve
         * their allocating seat from the canonical pair, just as contacts
         * do inside P2-owned AI. Otherwise a pending X type1B upload reads
         * Zero's packed data as a weapon directory, overruns PLAY/code in
         * native182D8 and aborts during the boss's death sequence. */
        return canonical_world_call(cpu,address,personal_deferred_effect_canonical);
    }
    return personal_deferred_effect_canonical(cpu,address);
}
#include "mmx5_coop_balance.inc"
static int whale_puffer(uint32_t actor) {
    return psx_mod_read_byte(PLAY+12)==3 && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==8 &&
        actor>=0x80092090u && actor<0x80093DD0u &&
        !((actor-0x80092090u)%0x9Cu) && psx_mod_read_byte(actor) &&
        psx_mod_read_byte(actor+1)==0x13 &&
        psx_mod_read_word(0x80100EA4u)==0x800F23B4u &&
        psx_mod_read_word(0x80101648u)==0x800F1B38u &&
        psx_mod_read_word(0x80101674u)==0x800F2978u;
}
static unsigned rose_capture_actor(uint32_t actor) {
    if(psx_mod_read_byte(PLAY+13) ||
       actor<0x80096D98u || actor>=0x80098118u ||
       (actor-0x80096D98u)%0x9Cu || !psx_mod_read_byte(actor))return 0;
    unsigned type=psx_mod_read_byte(actor+1),subtype=psx_mod_read_byte(actor+2);
    /* The refight uses a separate copy of the same native hand program.
     * Its contact leaf has the same immediate PLAYER.CA postcondition,
     * but its dispatcher and state tables live in BIN18 rather than BIN13. */
    if(psx_mod_read_byte(PLAY+12)==12 && psx_mod_read_word(0x800EE970u)==22 &&
       type==0x27 && !subtype &&
       psx_mod_read_word(0x800F6ECCu)==0x800FE0C8u &&
       psx_mod_read_word(0x800FEE54u)==0x800FE1ACu &&
       psx_mod_read_word(0x800FEE98u)==0x800FD6B0u &&
       psx_mod_read_word(0x800FD5B4u)==0x0C00C5EEu &&
       psx_mod_read_word(0x800FD5C4u)==0x822200CAu)return 2;
    if(psx_mod_read_byte(PLAY+12)!=7 || psx_mod_read_word(0x800EE970u)!=14)return 0;
    if(type==6 && subtype==4 &&
       psx_mod_read_word(0x800FADC8u)==0x800F2448u &&
       psx_mod_read_word(0x800F2EB4u)==0x0C00C5EEu &&
       psx_mod_read_word(0x800F2424u)==0xA06000CAu)return 1;
    if(type==0x27 && !subtype &&
       psx_mod_read_word(0x800FAE4Cu)==0x800F6EFCu &&
       psx_mod_read_word(0x800FBF8Cu)==0x800F6FE0u &&
       psx_mod_read_word(0x800FBFD0u)==0x800F64E4u)return 2;
    return 0;
}
static unsigned puffer_recipient(uint32_t actor,unsigned target) {
    if(!whale_puffer(actor))return target;
    HitHistory *h=hit_record(actor,0);
    if(!h || !h->capture_owner)return target;
    /* The parent follows PLAYER through F2A9C, and its retirement paths
     * release that same body's CA. Keep the actual captured recipient even
     * after native release, until this actor retires/reuses its slot. */
    unsigned seat=h->capture_owner-1;
    if(life.status[seat]!=MMX5_COOP_ALIVE)return target;
    if(target!=seat)psx_mod_counter_add("mmx5.coop.puffer-recipient-targets",1);
    return seat;
}
static int contact_canonical(CPUState *cpu,uint32_t address) {
    if(contact_call || (projected && !world_owner) || !enrolled || failed || psx_mod_read_byte(PLAY)!=10)return 0;
#ifndef PSX_NO_DEBUG_TOOLS
    if(fixture_damage_actor && !psx_mod_netplay_is_active() &&
       !psx_mod_local_view_scope() && address==0x80031DD0u &&
       cpu->gpr[4]==fixture_damage_actor) {
        uint32_t actor=fixture_damage_actor;fixture_damage_actor=0;
        if(psx_mod_read_byte(PLAY+12)==fixture_damage_stage &&
           psx_mod_read_word(PLAY+0x20)==actor && psx_mod_read_byte(actor) &&
           psx_mod_read_byte(actor+1)==fixture_damage_kind &&
           psx_mod_read_byte(actor+4)==1) {
            psx_mod_write_byte(actor+0x5C,0);
            cpu->gpr[2]=UINT32_MAX;
            psx_mod_counter_add("mmx5.coop.fixture-forced-boss-hits",1);
            return 1;
        }
    }
#endif
    contact_call=1;uint32_t actor=cpu->gpr[4],argument=cpu->gpr[5],result=0;
    unsigned seat=world_owner?1:0;
    unsigned rose_capture=rose_capture_actor(actor);
    int rose_query=rose_capture &&
        cpu->gpr[31]==(rose_capture==1?0x800F2EBCu:
                      psx_mod_read_byte(PLAY+12)==12?0x800FD5BCu:0x800F63F0u);
    if(address==0x800317B8u && ai_call && (rose_query || (whale_puffer(actor) &&
       psx_mod_read_byte(actor+4)==1 && psx_mod_read_byte(actor+0x62)==3 &&
       !psx_mod_read_byte(actor+0x60)))) {
        /* F1F00's capture query immediately tests its selected PLAYER.CA
         * at F1F10. Capturing the other body in a generic second contact
         * pass loses that postcondition and lets F2A9C follow a different
         * hero into terrain. Admit this one-recipient native grab on its
         * selected body. Rose's F2E90/F63CC have the same CA postcondition:
         * a secondary contact must not capture a different, unobserved body.
         * Ordinary damage and both heroes' shots stay on the
         * existing dual queries. Latch only a real native CA acquisition. */
        unsigned recipient=ai_owner,switched=recipient!=seat;
        if(switched)enter_second();
        uint8_t ca=psx_mod_read_byte(PLAYER+0xCA);
        if(life.status[recipient]==MMX5_COOP_ALIVE && !scene_passenger(recipient) &&
           !psx_mod_read_byte(PLAYER+0xCC))
            result=balance_contact(cpu,address,actor,argument,recipient);
        if(!ca && psx_mod_read_byte(PLAYER+0xCA) && psx_mod_read_byte(PLAYER+0x63)==3) {
            HitHistory *capture=hit_record(actor,1);
            if(capture)capture->capture_owner=(uint8_t)(recipient+1);
            if(rose_query)psx_mod_counter_add("mmx5.coop.rose-capture-admissions",1);
        }
        if(switched)leave_second();
        cpu->gpr[2]=result;contact_call=0;return 1;
    }
    unsigned shot_query=address==0x80031DD0u || address==0x80032F70u;
    HitHistory *h=shot_query?hit_record(actor,1):NULL;
    uint8_t stock_token=psx_mod_read_byte(actor+0x65);
    if(h)psx_mod_write_byte(actor+0x65,h->token[seat]);
    if(life.status[seat]==MMX5_COOP_ALIVE && !scene_passenger(seat) && !psx_mod_read_byte(PLAYER+0xCC)) {
        result=balance_contact(cpu,address,actor,argument,seat);
        h=shot_query?hit_record(actor,0):NULL;
        if(h)h->token[seat]=psx_mod_read_byte(actor+0x65);
    }
    /* Pegasus's timed-bomb blast makes one damage query, then retires its
     * collision phase unconditionally at EF4CC. A hit on P1 must not consume
     * P2's only query. Keep both native immunity/geometry checks and the
     * original first hit's return value; ordinary one-recipient contacts
     * and all shot queries retain their existing short circuit. */
    int paired_blast=address==0x800317B8u && cpu->gpr[31]==0x800EF4C4u &&
        psx_mod_read_byte(PLAY+12)==8 && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==0x0Fu &&
        psx_mod_read_word(0x800EF4BCu)==0x0C00C5EEu &&
        psx_mod_read_word(0x800EF4CCu)==0xA2030004u &&
        actor>=0x80096D98u && actor<0x80098118u &&
        !((actor-0x80096D98u)%0x9Cu) &&
        psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==0x2A &&
        psx_mod_read_byte(actor+4)==1 && psx_mod_read_byte(actor+0x60)==20;
    if(paired_blast) {
        uint32_t parent=psx_mod_read_word(actor+0x7C);
        paired_blast=parent>=0x80092090u && parent<0x80093DD0u &&
            !((parent-0x80092090u)%0x9Cu) && psx_mod_read_byte(parent) &&
            psx_mod_read_byte(parent+1)==0x1F && psx_mod_read_half(parent+0x82)==1;
    }
    if(!world_owner && (!result || paired_blast) && life.status[1]==MMX5_COOP_ALIVE && !scene_passenger(1) &&
       (second.body[0x5C]&0x7F) && !second.body[0xCC]) {
        if(h)psx_mod_write_byte(actor+0x65,h->token[1]);
        uint32_t primary_result=result;
        enter_second();result=balance_contact(cpu,address,actor,argument,1);leave_second();
        h=shot_query?hit_record(actor,0):NULL;
        if(h)h->token[1]=psx_mod_read_byte(actor+0x65);
        if(result) {
            psx_mod_counter_add(shot_query?"mmx5.coop.p2-hits":"mmx5.coop.p2-contact",1);
            psx_mod_write_word(diagnostic+0x68,actor);psx_mod_write_word(diagnostic+0x6C,result);
        }
        if(primary_result)result=primary_result;
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
static int visible_downlift_rider(uint32_t actor) {
    /* This retained passenger boarded before the original down-lift scene.
     * F4B9C lowers the support while the stand command still holds the pair.
     * Its real solid pass must carry the visible companion too; otherwise
     * scene_tick sees the old foot above the new top and beams P2 away. */
    return scene_owner && !scene_phase &&
        psx_mod_read_byte(PLAY+12)==8 && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==0x0Fu &&
        psx_mod_read_word(0x800F9C6Cu)==0x800F48B4u &&
        psx_mod_read_byte(actor+1)==0x12 && !psx_mod_read_byte(actor+2) &&
        psx_mod_read_byte(actor+4)==1 && psx_mod_read_byte(actor+5)==5;
}
static int solid_contact_canonical(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];SolidHistory *h=solid_record(actor);
    if(solid_call || !h || !enrolled || failed || (projected && !world_owner))return 0;
    if(projected && world_owner && scene_owner==2 && visible_downlift_rider(actor))
        return canonical_world_call(cpu,address,solid_contact_canonical);
    solid_call=1;uint32_t result;
    if(world_owner)result=solid_as_second(cpu,address,actor,h);
    else {
        result=guest(cpu,address,actor,cpu->gpr[5]);
        if(life.status[1]==MMX5_COOP_ALIVE && second.body[4]==1 &&
           (!scene_passenger(1) || visible_downlift_rider(actor))) {
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
static void broken_hover_rider(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[16];SolidHistory *h=solid_record(actor);
    unsigned stage=psx_mod_read_byte(PLAY+12),overlay=psx_mod_read_word(0x800EE970u);
    int opening=address==0x800F4620u && !stage && overlay==4 &&
        psx_mod_read_word(0x800FA58Cu)==0x800F4BC4u &&
        psx_mod_read_word(0x800FAFC4u)==0x800F45BCu;
    int sigma2=address==0x800F7744u && stage==17 && overlay==31 &&
        psx_mod_read_word(0x800FD6DCu)==0x800F76E0u &&
        psx_mod_read_word(actor+0x68)==0x800FD5B8u;
    int sigma3=address==0x800FC6A0u && stage==18 && overlay==0x20 &&
        psx_mod_read_word(0x800FEA10u)==0x800FC63Cu &&
        psx_mod_read_word(actor+0x68)==0x800FE8ECu;
    if(cpu->gpr[3] || !h || (projected && !world_owner) || busy || !enrolled || failed ||
       psx_mod_local_view_scope() || scene_owner || life.status[1]!=MMX5_COOP_ALIVE ||
       second.body[4]!=1 || psx_mod_read_byte(PLAY)!=10 ||
       psx_mod_read_byte(PLAY+13) || (!opening && !sigma2 && !sigma3) ||
       psx_mod_read_byte(actor+1) || psx_mod_read_byte(actor+2)!=3 ||
       psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+6)!=3 ||
       !(h->sides&8u) || !h->carried)return;
    /* Native damaged pods read their previous 3240C rider byte before
     * advancing their fall delay and acceleration. P2 retains a private byte;
     * include it in this predicate without replacing P1's collision history
     * or running the shared platform, timer or effects a second time. The
     * survivor world also projects P2, so it must admit that same history. */
    cpu->gpr[3]=(uint32_t)(int32_t)(int8_t)h->carried;
    psx_mod_counter_add(sigma2?"mmx5.coop.p2-sigma2-hover-riders":
                        sigma3?"mmx5.coop.p2-sigma3-hover-riders":
                              "mmx5.coop.p2-intro-hover-riders",1);
}
static int capsule_near(uint32_t actor,int x,int y) {
    int dx=(int16_t)(x-(int16_t)psx_mod_read_half(actor+10));
    int dy=(int16_t)(y-(int16_t)psx_mod_read_half(actor+14));
    return dx>=-64 && dx<=64 && dy>=-104 && dy<=16;
}
static int capsule_secret_eligible(unsigned character,unsigned armor) {
    unsigned flags=psx_mod_read_byte(PLAY+0x4A);
    /* Original54FE4: Zero needs the uncollected Black flag; X must be
     * unarmored and have neither the Ultimate grant nor its native code. */
    return character==1?!(flags&0x10u):
        character==0 && !armor && !(flags&8u) && !psx_mod_read_byte(PLAY+0x4B);
}
static int capsule_secret_init(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(area_call || busy || projected || !enrolled || failed ||
       psx_mod_local_view_scope() || scene_owner || car_context() ||
       psx_mod_read_byte(PLAY)!=10 ||
       actor<0x800C6A10u || actor>=0x800C9310u || (actor-0x800C6A10u)%0xA4u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x1A ||
       psx_mod_read_byte(actor+2)!=8 ||
       life.status[1]!=MMX5_COOP_ALIVE || second.body[4]!=1 ||
       !capsule_secret_eligible(second.body[2],second.body[0xFA]) ||
       psx_mod_read_word(address)!=0x27BDFFD8u)return 0;
    if(life.status[0]==MMX5_COOP_ALIVE && psx_mod_read_byte(PLAYER+4)==1 &&
       capsule_secret_eligible(psx_mod_read_byte(PLAYER+2),
                               psx_mod_read_byte(PLAYER+0xFA)))return 0;
    /* Initial admission precedes the proximity gate. Fourth Armor on P1
     * must not retire Zero/P2's still-uncollected capsule. Run the original
     * initializer as that eligible hero, keeping shared art and progress. */
    area_call=1;
    uint8_t campaign=psx_mod_read_byte(PLAY+0x44);enter_second();world_owner=1;
    psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    world_owner=0;leave_second_context(1);area_call=0;
    psx_mod_counter_add("mmx5.coop.p2-secret-capsule-admission",1);
    cpu->gpr[2]=result;return 1;
}
static int capsule_black_form(CPUState *cpu,uint32_t address) {
    (void)address;uint32_t actor=cpu->gpr[4];
    /* Original55D24 awards subtype8 to the actual recipient. Select Black
     * before its native flag store/palette reinitialization, without setting
     * progress ourselves. X's Ultimate grant remains on its original branch. */
    if(!enrolled || failed || busy || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAYER+2)!=1 || psx_mod_read_byte(PLAY)!=10 ||
       actor<0x800C6A10u || actor>=0x800C9310u || (actor-0x800C6A10u)%0xA4u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x1A ||
       psx_mod_read_byte(actor+2)!=8 ||
       psx_mod_read_word(0x80055D94u)!=0x34420010u)return 0;
    selected_zero_form=1;
    psx_mod_counter_add("mmx5.coop.black-zero-capsule-grants",1);
    /* The shared capsule updates retain P1's whole-stage palette. Only the
     * recipient's native palette reinitialization belongs in P2's buffer. */
    uint32_t palette=psx_mod_read_word(SCRATCH+0x28);busy=1;
    if(projected)psx_mod_write_word(SCRATCH+0x28,assets[counterpart].palette);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    /* The original grant changes Zero's palette but never reruns the stage
     * entry equipment load. A mid-stage form change must select the same
     * persistent Black Armor row as native3D608 does on the next entry.
     * Keep both saved rows and every other live body field unchanged. */
    if(psx_mod_read_byte(PLAY+0x4A)&0x10u)
        psx_mod_write_word(PLAYER+0xFC,psx_mod_read_word(PLAY+0x94));
    psx_mod_write_word(SCRATCH+0x28,palette);busy=0;
    cpu->gpr[2]=result;return 1;
}
static uint8_t both_body_byte(unsigned seat,unsigned at);
static int capsule_secret_return(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(area_call || busy || !enrolled || failed || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=18 ||
       psx_mod_read_byte(PLAY+13) ||
       actor<0x800C6A10u || actor>=0x800C9310u || (actor-0x800C6A10u)%0xA4u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x1A ||
       psx_mod_read_byte(actor+2)!=8 || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+5)!=11)return 0;
    int eligible=0;
    for(unsigned seat=0;seat<2;++seat)
        if(life.status[seat]==MMX5_COOP_ALIVE && both_body_byte(seat,4)==1 &&
           capsule_secret_eligible(both_body_byte(seat,2),both_body_byte(seat,0xFA)))eligible=1;
    if(!eligible)return 0;
    /* The original grant/return must finish before another recipient can
     * enter. Phase11 already acknowledges the first grant to scene_tick.
     * Keep its controller until that actual player's return releases it. */
    if(scene_owner) {cpu->gpr[2]=0;return 1;}
    /* Original55DEC retains the spent capsule until it leaves the camera;
     * 2D938 is its offscreen test. Reinitialize this same placement now that
     * the real recipient has returned. Waiting for that cull would require
     * leaving the area before the second hero could use it. */
    psx_mod_write_byte(actor+4,0);psx_mod_write_byte(actor+5,0);
    psx_mod_write_byte(actor+6,0);
    psx_mod_counter_add("mmx5.coop.secret-capsule-rearmed",1);
    cpu->gpr[2]=0;return 1;
}
static int capsule_text(CPUState *cpu,uint32_t address) {
    unsigned id=cpu->gpr[4]&0x7FFFu;
    if(area_call || busy || !ready || !enrolled || failed || !scene_owner ||
       psx_mod_local_view_scope() || psx_mod_read_byte(PLAY)!=10)return 0;
    int survivor_boss=scene_owner==2 && !both_living(0) && both_living(1) &&
        psx_mod_read_byte(PLAY+12)==3 && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==8 && (id==0x2E || id==0x60) &&
        psx_mod_read_word(0x800F50A0u)==0x0C008E17u &&
        psx_mod_read_word(0x800F50B4u)==0x0C008E17u;
    if(!survivor_boss && id!=0x55 && (id<0x3A || id>0x41))return 0;
    unsigned seat=scene_owner-1,character;
    int x,y;
    if(seat==world_owner) {
        character=psx_mod_read_byte(PLAYER+2);
        x=(int16_t)psx_mod_read_half(PLAYER+10);y=(int16_t)psx_mod_read_half(PLAYER+14);
    }else {
        const uint8_t *body=seat?second.body:first.body;
        character=body[2];x=(int16_t)(le32(body+8)>>16);y=(int16_t)(le32(body+12)>>16);
    }
    if(character>1 || life.status[seat]!=MMX5_COOP_ALIVE)return 0;
    if(survivor_boss) {
        /* Story progression still belongs to the saved campaign. Bind only
         * this surviving participant's message cursor, including a queued
         * replay of the same native introduction, to its character bank. */
        uint32_t root=psx_mod_read_word(SCRATCH+0x48);area_call=1;
        psx_mod_write_word(SCRATCH+0x48,assets[character].message_root);
        uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        psx_mod_write_word(SCRATCH+0x48,root);area_call=0;
        psx_mod_counter_add("mmx5.coop.survivor-boss-text",1);
        cpu->gpr[2]=result;return 1;
    }
    for(unsigned i=0;i<64;++i) {
        uint32_t actor=0x800C6A10u+i*0xA4u;
        unsigned subtype=psx_mod_read_byte(actor+2),phase=psx_mod_read_byte(actor+5);
        if(!psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x1A || subtype>8 ||
           psx_mod_read_byte(actor+4)!=1 || (phase!=2 && phase!=3) ||
           (subtype==8?0x55u:psx_mod_read_byte(0x8007C3B0u+subtype))!=id ||
           !capsule_near(actor,x,y))continue;
        /* 2385C binds a persistent cursor into the original message bank.
         * Scope the recipient bank at actual initialization, including a
         * queued request replay, rather than changing global story resources
         * or rewriting the author's text. Deferred pages retain this cursor. */
        uint32_t root=psx_mod_read_word(SCRATCH+0x48);area_call=1;
        psx_mod_write_word(SCRATCH+0x48,assets[character].message_root);
        uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        psx_mod_write_word(SCRATCH+0x48,root);area_call=0;
        psx_mod_counter_add("mmx5.coop.capsule-recipient-text",1);
        cpu->gpr[2]=result;return 1;
    }
    return 0;
}
static int capsule_top_geometry(uint32_t actor,const uint8_t *body) {
    uint32_t box=le32(body+0x68),solid=psx_mod_read_word(actor+0x68);
    if(body[4]!=1 || box<0x80010000u || box>0x801FFFFCu ||
       solid<0x80010000u || solid>0x801FFFFCu)return 0;
    int offset=(int8_t)psx_mod_read_byte(box);
    int x=(int16_t)(le32(body+8)>>16)+(body[0x15]==0x40?-offset:offset);
    int foot=(int16_t)(le32(body+12)>>16)+(int8_t)psx_mod_read_byte(box+1)+
        psx_mod_read_byte(box+3);
    offset=(int8_t)psx_mod_read_byte(solid);
    int center=(int16_t)psx_mod_read_half(actor+10)+
        (psx_mod_read_byte(actor+0x15)==0x40?-offset:offset);
    int top=(int16_t)psx_mod_read_half(actor+14)+(int8_t)psx_mod_read_byte(solid+1)-
        psx_mod_read_byte(solid+3);
    int reach=psx_mod_read_byte(box+2)+psx_mod_read_byte(solid+2);
    return foot==top && x>center-reach && x<center+reach;
}
static int capsule_gate(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];SolidHistory *h=solid_record(actor);
    /* A rearmed secret capsule belongs only to an uncollected recipient.
     * The completed hero can remain beside it without taking admission. */
    int secret=actor>=0x800C6A10u && actor<0x800C9310u &&
        !((actor-0x800C6A10u)%0xA4u) && psx_mod_read_byte(actor+1)==0x1A &&
        psx_mod_read_byte(actor+2)==8;
    int primary_eligible=!secret || capsule_secret_eligible(psx_mod_read_byte(PLAYER+2),
                                                         psx_mod_read_byte(PLAYER+0xFA));
    int resident_top=1;
    if(secret && address==0x80055AD0u && enrolled && !failed) {
        uint8_t body[0x158];capture(PLAYER,body,sizeof body);
        resident_top=primary_eligible && capsule_top_geometry(actor,body);
        int partner_top=!projected && h && (h->sides&8u) &&
            life.status[1]==MMX5_COOP_ALIVE &&
            capsule_secret_eligible(second.body[2],second.body[0xFA]) &&
            capsule_top_geometry(actor,second.body);
        /* A collected hero can remain on the capsule while its partner
         * approaches at ground level. Neither that hero's native side byte
         * nor a retained pre-transport contact admits the other recipient. */
        if(!resident_top && !partner_top) {cpu->gpr[2]=0;return 1;}
    }
    if(secret && enrolled && !failed && address==0x80055798u && !primary_eligible &&
       (projected || life.status[1]!=MMX5_COOP_ALIVE ||
        !capsule_secret_eligible(second.body[2],second.body[0xFA]) ||
        !capsule_near(actor,(int16_t)(second.body[10]|second.body[11]<<8),
                          (int16_t)(second.body[14]|second.body[15]<<8)))) {
        cpu->gpr[2]=0;return 1;
    }
    if(area_call || busy || !enrolled || failed || psx_mod_local_view_scope() ||
       (projected && !world_owner) || !h || car_context() ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(actor+1)!=0x1A ||
       psx_mod_read_byte(actor+2)>8 || psx_mod_read_byte(actor+4)!=1 ||
       life.status[1]!=MMX5_COOP_ALIVE || second.body[4]!=1 ||
       (scene_owner && scene_owner!=2))return 0;
    if(address==0x80055798u) {
        if(projected || scene_owner)return 0;
        if(primary_eligible && life.status[0]==MMX5_COOP_ALIVE && capsule_near(actor,
           (int16_t)psx_mod_read_half(PLAYER+10),(int16_t)psx_mod_read_half(PLAYER+14)))return 0;
        if(!capsule_near(actor,(int16_t)(second.body[10]|second.body[11]<<8),
                        (int16_t)(second.body[14]|second.body[15]<<8)))return 0;
    }else {
        /* After the Light conversation, original55AD0 requires the previous
         * actual top contact. The shared actor stores P1's sides outside a
         * contact call, even when P2 already owns the native stage pass. */
        if((!projected && life.status[0]==MMX5_COOP_ALIVE && resident_top &&
            (psx_mod_read_byte(actor+0x72)&8)) || !(h->sides&8))return 0;
    }
    /* Ordinary X5 programs accept either hero. Project only these original
     * admission gates; the capsule still owns text, animation, actual grant
     * and native return. Preserve the campaign and full shared palette. */
    area_call=1;unsigned own_scope=!projected;
    uint8_t sides=psx_mod_read_byte(actor+0x72);
    if(own_scope) {
        uint8_t campaign=psx_mod_read_byte(PLAY+0x44);enter_second();world_owner=1;
        psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
    }
    if(address==0x80055AD0u)psx_mod_write_byte(actor+0x72,h->sides);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    psx_mod_write_byte(actor+0x72,sides);
    if(own_scope) {world_owner=0;leave_second_context(1);}
    area_call=0;
    psx_mod_counter_add(address==0x80055798u?"mmx5.coop.p2-capsule-proximity":
                                                   "mmx5.coop.p2-capsule-contact",1);
    cpu->gpr[2]=result;return 1;
}
static int capsule_return_committed(const uint8_t *body) {
    if(body[0xD1]!=0x18)return 0;
    int x=(int16_t)(le32(body+8)>>16),y=(int16_t)(le32(body+12)>>16);
    for(unsigned i=0;i<64;++i) {
        uint32_t actor=0x800C6A10u+i*0xA4u;
        unsigned subtype=psx_mod_read_byte(actor+2);
        if(!psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x1A || subtype>8 ||
           psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+5)!=11 ||
           !capsule_near(actor,x,y))continue;
        if(subtype==8) {
            if(body[2]==1?(psx_mod_read_byte(PLAY+0x4A)&0x10u):
                          psx_mod_read_byte(PLAY+0x4B))return 1;
            continue;
        }
        unsigned mask=psx_mod_read_byte(0x8007C370u+subtype);
        if(psx_mod_read_byte(PLAY+0xA1)&mask)return 1;
    }
    return 0;
}
static uint32_t capsule_return_scene(const uint8_t *body) {
    /* Light releases stand14 after the text, before the optional action18
     * grant. Both healthy returns belong to the same native capsule; neither
     * should manufacture a new injury timer on its companion. */
    if(body[0xD1]!=0x14 && body[0xD1]!=0x18)return 0;
    int x=(int16_t)(le32(body+8)>>16),y=(int16_t)(le32(body+12)>>16);
    for(unsigned i=0;i<64;++i) {
        uint32_t actor=0x800C6A10u+i*0xA4u;
        unsigned phase=psx_mod_read_byte(actor+5);
        if(psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==0x1A &&
           psx_mod_read_byte(actor+2)<=8 && psx_mod_read_byte(actor+4)==1 &&
           phase>=2 && phase<=11 && capsule_near(actor,x,y))return actor;
    }
    return 0;
}
static int capsule_return_pending(uint32_t actor,const uint8_t *body) {
    if(actor<0x800C6A10u || actor>=0x800C9310u ||
       (actor-0x800C6A10u)%0xA4u || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=0x1A || psx_mod_read_byte(actor+2)>8 ||
       psx_mod_read_byte(actor+4)!=1)return 0;
    unsigned phase=psx_mod_read_byte(actor+5);
    return phase>=2 && phase<11 && capsule_near(actor,
        (int16_t)(le32(body+8)>>16),(int16_t)(le32(body+12)>>16));
}
static void pickup_armor_collider(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!pickup_call || !enrolled || failed || psx_mod_local_view_scope() ||
       !native_armor_overlay())return;
    unsigned rider=psx_mod_read_byte(PLAYER+0xD5),slot;
    if(rider==1 || rider==255)slot=0;
    else if(rider==2 || rider==254)slot=1;
    else return;
    uint32_t armor=CAR+slot*0xB0u;
    if(!psx_mod_read_byte(armor) || psx_mod_read_byte(armor+1)!=1)return;
    /* Native54030 guesses the occupied armor from slot0's action. Both
     * original slots can now be occupied, so use this rider's native D5
     * slot marker for the existing health and overlap admission instead.
     * Pickup effects, resources and retirement remain entirely native. */
    if(cpu->gpr[5]!=armor) {
        cpu->gpr[5]=armor;
        psx_mod_counter_add("mmx5.coop.armor-pickup-collider",1);
    }
}
static uint32_t pickup_native(CPUState *cpu,uint32_t address,uint32_t actor) {
    unsigned kind=psx_mod_read_byte(actor+2);
    int health=psx_mod_read_byte(actor+1)==0x2F && kind>=0x20 && kind<=0x22;
    uint8_t locks[8],effect_lock=0;
    if(health) {capture(PLAY+0x10,locks,sizeof locks);effect_lock=psx_mod_read_byte(PLAY+0x19);}
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    if(health) {
        /* Original53E3C/53A4C uses545DC to stop every world category while
         * a small, large or full-health item fills its collector. Co-op retains the
         * native queue/sound/resource updates, but preserves the surrounding
         * locks so both heroes and the world continue. Completion likewise
         * cannot clear a genuine lock that another scene has since acquired. */
        project(PLAY+0x10,locks,sizeof locks);psx_mod_write_byte(PLAY+0x19,effect_lock);
    }
    return result;
}
static uint32_t pickup_as_seat(CPUState *cpu,uint32_t address,uint32_t actor,unsigned seat) {
    if(seat==world_owner)return pickup_native(cpu,address,actor);
    etanks_capture(world_owner);etanks_project(seat);
    uint8_t body[0x158],character=psx_mod_read_byte(PLAY+0x44);
    uint8_t *other=seat?second.body:first.body;
    capture(PLAYER,body,sizeof body);project(PLAYER,other,sizeof body);
    psx_mod_write_byte(PLAY+0x44,other[2]);
    uint32_t result=pickup_native(cpu,address,actor);
    capture(PLAYER,other,sizeof body);project(PLAYER,body,sizeof body);
    etanks_capture(seat);etanks_project(world_owner);
    psx_mod_write_byte(PLAY+0x44,character);return result;
}
static int pickup_collect_canonical(CPUState *cpu,uint32_t address) {
    if(pickup_call || !enrolled || failed)return 0;
    if(duel_scope()) {cpu->gpr[2]=0;return 1;}
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
static int scene_companion_grounded(unsigned seat) {
    uint8_t body[0x158];
    if(seat==projected)capture(PLAYER,body,sizeof body);
    else memcpy(body,seat?second.body:first.body,sizeof body);
    return life.status[seat]==MMX5_COOP_ALIVE && body[4]==1 &&
        !body[0x67] && !body[0xD5] && le32(body+0x68) &&
        ((body[0x70]&8u) || body_on_support(seat,body));
}
static int scene_companion_visible(unsigned seat) {
    const uint8_t *body=seat?second.body:first.body;
    int x=seat==projected?(int16_t)psx_mod_read_half(PLAYER+10):
        (int16_t)(le32(body+8)>>16);
    int y=seat==projected?(int16_t)psx_mod_read_half(PLAYER+14):
        (int16_t)(le32(body+12)>>16);
    int left=(int16_t)psx_mod_read_half(CAMERA+10),top=(int16_t)psx_mod_read_half(CAMERA+14);
    return x>=left-24 && x<left+344 && y>=top-24 && y<top+264;
}
static void scene_begin(unsigned seat) {
    /* Both native cars stop with the one world/script lock. A rider cannot
     * use the ordinary passenger beam while its vehicle remains behind. */
    if(car_context())return;
    if(!enrolled || failed || scene_owner || seat>1 || !both_living(seat) ||
       psx_mod_local_view_scope() || both_door_active())return;
    int crossed_pair=both_door_scope() && both_door.phase==4 &&
        both_living(0) && both_living(1);
    if(split_views && split_valid && split_owner!=seat+1) {
        project(CAMERA,split_cameras[seat],sizeof saved_camera);split_owner=seat+1;
    }
    int paired_intro=crossed_pair;
    if(crossed_pair) {
        /* Crossing the aperture does not imply reaching the arena: Grizzly's
         * entrance drops a full screen below its upper ledge. Preserve the
         * no-beam introduction only when the companion is in the owner's
         * actual view; otherwise use the ordinary safe passenger arrival. */
        unsigned passenger=seat^1u;
        paired_intro=scene_companion_visible(passenger) &&
            (scene_companion_grounded(passenger) || duel_scope());
        both_door.phase=0;
        psx_mod_counter_add(paired_intro?"mmx5.coop.both-door-intro-without-warp":
                                         "mmx5.coop.both-door-intro-with-warp",1);
    }
    scene_corridor_arrow=0;scene_capsule=0;
    scene_owner=seat+1;scene_phase=both_living(seat^1) && !paired_intro && !paired_victory?1:0;
    life.select_frames=life.select_armed=0;
    psx_mod_counter_add("mmx5.coop.scene-starts",1);
}
static int paired_victory_scene(CPUState *cpu) {
    unsigned stage=psx_mod_read_byte(PLAY+0x0C);
    uint32_t actor=psx_mod_read_word(SCRATCH+0x4C);
    /* Native Maverick death holds command14 before setting PLAY+0F's victory
     * bit. Only its active, zero-HP boss/health-bank actor can retain the pair;
     * ordinary stand commands, doors, mid-stage scripts and refights keep
     * their existing transport policy. No clear or victory flag is written. */
    if(cpu->gpr[4]!=0x14u || scene_owner || both_door_active() || car_context() ||
       life.status[0]!=MMX5_COOP_ALIVE || life.status[1]!=MMX5_COOP_ALIVE ||
       actor<0x80092090u || actor>=0x80093DD0u || (actor-0x80092090u)%0x9Cu ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+4)!=2 ||
       (psx_mod_read_byte(actor+0x5C)&127u) || psx_mod_read_word(PLAY+0x20)!=actor)return 0;
    if(stage>=1 && stage<=8)return 1;
    /* BIN38's first final-body death also issues stand14 before its native
     * retirement/next-module sequence. Keep both bodies through that hold;
     * its initialization, refights and BIN39's ending retain their policies. */
    return stage==12 && !psx_mod_read_byte(PLAY+0x0D) &&
        psx_mod_read_word(0x800EE970u)==0x16u &&
        psx_mod_read_word(0x800FA000u)==0x2Au &&
        psx_mod_read_byte(actor+1)==0x42u && !psx_mod_read_byte(actor+2) &&
        !psx_mod_read_byte(actor+5) && cpu->gpr[31]==0x800FB570u &&
        psx_mod_read_word(0x800FD890u)==0x800FB514u &&
        psx_mod_read_word(0x800FD8CCu)==0x800FB550u &&
        psx_mod_read_word(0x800FB568u)==0x0C00F4A7u &&
        psx_mod_read_word(0x800FB56Cu)==0x24040014u;
}
static int support_pair_on_top(uint32_t actor) {
    SolidHistory *h=solid_record(actor);
    return h && life.status[0]==MMX5_COOP_ALIVE && life.status[1]==MMX5_COOP_ALIVE &&
        (psx_mod_read_byte(actor+0x72)&8u) && (h->sides&8u);
}
static int down_support_scene_riders(CPUState *cpu) {
    uint32_t actor=cpu->gpr[16],caller=cpu->gpr[31];
    if(psx_mod_read_byte(PLAY+12)!=8 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=0x0Fu ||
       psx_mod_read_word(0x800F9C6Cu)!=0x800F48B4u ||
       psx_mod_read_word(0x800F48D8u)!=0x0C00F4A7u ||
       psx_mod_read_word(0x800F4AF8u)!=0x0C00F4A7u ||
       !solid_record(actor) || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=0x12 || psx_mod_read_byte(actor+2) ||
       psx_mod_read_byte(actor+4)!=1 || !support_pair_on_top(actor))return 0;
    return (caller==0x800F48E0u && psx_mod_read_byte(actor+5)==2) ||
        (caller==0x800F4B00u && psx_mod_read_byte(actor+5)==5 && !psx_mod_read_byte(actor+6));
}
static int whale_authored_camera(void);
static void script_begin(CPUState *cpu,uint32_t address) {
    (void)address;
    if(psx_mod_local_view_scope())return;
    /* BIN18's hub controller briefly stands its arriving hero and releases
     * that same command on its next update. It is an independent room
     * arrival, not a boss teleporter or shared cutscene. Keep the original
     * entrant's command without gathering a distant companion. */
    if(cpu->gpr[31]==0x800EEE90u && psx_mod_read_byte(PLAY+12)==12 &&
       !psx_mod_read_byte(PLAY+13) && psx_mod_read_word(0x800EE970u)==22u &&
       psx_mod_read_word(0x800EEE88u)==0x0C00F4A7u &&
       psx_mod_read_word(0x800EEEA8u)==0x0C00F4B1u)return;
    if(!scene_call && (!projected || world_owner)) {
        if(paired_victory_scene(cpu)) {
            paired_victory=1;psx_mod_counter_add("mmx5.coop.paired-victory-scenes",1);
        }
        unsigned previous=scene_owner;
        scene_begin(world_owner);
        if(cpu->gpr[31]==0x800FC548u && whale_authored_camera() &&
           psx_mod_read_word(0x800FC540u)==0x0C00F4A7u &&
           (psx_mod_read_byte(PLAY+0x2A)&7u)==7u) {
            /* First-entry whale scrolling leaves the checkpoint's ordinary
             * Y limits at240. This original last-bank stand enables normal
             * follow, so29184 otherwise snaps back up and kills its owner.
             * Commit the observed authored viewport ONCE at that command;
             * updating from the shaken camera each frame accumulates shake.
             * Native room/player clamps, hazards and retirement still run. */
            uint16_t y=psx_mod_read_half(CAMERA+14);
            psx_mod_write_half(CAMERA+0x20,y);psx_mod_write_half(CAMERA+0x22,y);
            psx_mod_write_half(CAMERA+0x28,y);psx_mod_write_half(CAMERA+0x2A,y);
        }
        if(!previous && scene_owner && down_support_scene_riders(cpu)) {
            /* Both heroes boarded this real stationary lift before its
             * native stand/walk or stop command. Keep the companion on it:
             * the existing visible-scene path mirrors those commands and
             * original3240C carries both bodies. A beam would return it at
             * the camera's upper edge after the lift has already descended. */
            scene_phase=0;
            psx_mod_counter_add("mmx5.coop.lift-scene-without-warp",1);
        }
        /* A nearby partner can retain its place. A distant partner needs
         * the ordinary scene departure/return: the shared hint camera must
         * never apply its room bounds to that remote living body. */
        if(!previous && scene_owner && cpu->gpr[31]==0x80056D64u &&
           psx_mod_read_word(0x80056D5Cu)==0x0C00F4A7u) {
            unsigned passenger=(scene_owner-1u)^1u;
            const uint8_t *body=passenger?second.body:first.body;
            int x=passenger==projected?(int16_t)psx_mod_read_half(PLAYER+10):
                (int16_t)(le32(body+8)>>16);
            int y=passenger==projected?(int16_t)psx_mod_read_half(PLAYER+14):
                (int16_t)(le32(body+12)>>16);
            int left=(int16_t)psx_mod_read_half(CAMERA+10),top=(int16_t)psx_mod_read_half(CAMERA+14);
            if(x>=left+16 && x<left+304 && y>=top+8 && y<top+232 &&
               scene_companion_grounded(passenger))scene_phase=0;
            else psx_mod_counter_add("mmx5.coop.distant-hint-gathers",1);
        }
    }
}
static int partner_victory_pose(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!paired_victory || !scene_call || !enrolled || failed ||
       psx_mod_local_view_scope() || cpu->gpr[4]!=PLAYER ||
       !(psx_mod_read_byte(PLAY+0x0F)&0x10u))return 0;
    /* Match native3B1A8's character cleanup and action25 initialization.
     * The world owner retains its one original177B0 shared music request;
     * the companion animates its own native victory and outgoing beam. */
    guest(cpu,0x8003CE64,PLAYER,0);guest(cpu,0x8003EC00,PLAYER,0);
    psx_mod_write_byte(PLAY+0x1C,1);
    guest(cpu,0x8003D0A0,PLAYER,0);guest(cpu,0x8003ED10,PLAYER,0);
    if(psx_mod_read_byte(PLAYER+2))psx_mod_write_byte(PLAYER+0x93,0);
    psx_mod_write_byte(PLAYER+5,0x19);psx_mod_write_byte(PLAYER+6,0);
    psx_mod_counter_add("mmx5.coop.paired-victory-poses",1);
    cpu->gpr[2]=0x19;return 1;
}
void mmx5_coop_silent_hint(void) {
    /* Entry callbacks can observe3D29C before the selected Alia filter
     * suppresses that one call. No passenger tick has run at this point.
     * Cancel the provisional scene; spatial_stage_actor restores its scoped
     * camera normally. Existing scenes and victory poses are untouched. */
    if(enrolled && !paired_victory && scene_owner==world_owner+1 && scene_phase<=1)
        scene_owner=scene_phase=0;
}
static void paired_victory_tick(CPUState *cpu,unsigned owner,const uint8_t *leader) {
    unsigned companion=owner^1u;
    if(life.status[companion]!=MMX5_COOP_ALIVE)return;
    uint8_t play[sizeof saved_play],camera[sizeof saved_camera];
    capture(PLAY,play,sizeof play);capture(CAMERA,camera,sizeof camera);
    unsigned old_busy=busy;busy=scene_call=1;
    if(companion)enter_second();
    /* The companion's attack dispatcher is suspended by the shared defeat
     * hold. Retire its old projectiles instead of drawing their last frame
     * through palette/asset changes and the victory pose. Boss explosions
     * and other world effects retain their original update and lifetime. */
    unsigned retired=0;
    for(unsigned i=0;i<32;++i)if(psx_mod_read_byte(SHOTS+i*0x9Cu)) {
        psx_mod_write_byte(SHOTS+i*0x9Cu,0);
        psx_mod_write_byte(SHOTS+i*0x9Cu+3,0);++retired;
    }
    if(retired) {
        clear_buster_effects(companion);
        psx_mod_write_byte(PLAYER+0x98,0);psx_mod_write_byte(PLAYER+0x99,0);
        psx_mod_write_byte(PLAYER+0xA6,0);
        psx_mod_counter_add("mmx5.coop.defeat-projectiles-retired",retired);
    }
    if(paired_victory==1) {
        /* Replay the observed original stand/facing command once, rather
         * than transporting the still-living companion out of the arena. */
        guest(cpu,0x8003D29C,leader[0xD1],leader[0xD2]);paired_victory=2;
    }
    guest(cpu,0x80035040,0,0);
    if(companion)leave_second();
    /* The owner's native stage pass alone commits world clocks, music and
     * progression. Retain only the companion's own body/animation changes. */
    project(PLAY,play,sizeof play);project(CAMERA,camera,sizeof camera);
    scene_call=0;busy=old_busy;
}
static void scene_recover_owner(CPUState *cpu) {
    if(!scene_owner || !scene_phase || !enrolled || failed || busy || projected || car_context())return;
    unsigned owner=scene_owner-1,survivor=owner^1;
    if((life.status[owner]!=MMX5_COOP_DYING && life.status[owner]!=MMX5_COOP_FALLEN) ||
       life.status[survivor]!=MMX5_COOP_ALIVE)return;
    uint8_t leader[0x158],body[0x158];
    if(owner)memcpy(leader,second.body,sizeof leader);else capture(PLAYER,leader,sizeof leader);
    if(survivor)memcpy(body,second.body,sizeof body);else capture(PLAYER,body,sizeof body);
    if(!(body[0x5C]&0x7F))return;
    if(scene_phase==2 || scene_phase==3)memcpy(body,scene_body,sizeof body);
    /* A mounted passenger requires its vehicle's native transfer/dismount,
     * rather than relocating only the rider. This path restores foot actors. */
    if(body[0xD5])return;
    unsigned old_busy=busy,old_scene_call=scene_call;busy=scene_call=1;
    clear_buster_effects(owner);clear_buster_effects(survivor);
    clear_personal_drawables(owner);clear_personal_drawables(survivor);
    if(owner)enter_second();
    uint32_t old_box=psx_mod_read_word(PLAYER+0x68);
    /* Native fatal entry clears the box. 3CD08 only refreshes +68; probe its
     * original foot box and restore the corpse's collisionless state. */
    guest(cpu,0x8003CD08,PLAYER,0);uint32_t leader_box=psx_mod_read_word(PLAYER+0x68);
    psx_mod_write_word(PLAYER+0x68,old_box);
    psx_mod_write_byte(PLAYER+0xD0,0);psx_mod_write_byte(PLAYER+0xD4,0);
    psx_mod_write_byte(PLAYER+0x7A,0);psx_mod_write_byte(PLAYER+0x67,0);
    if(owner)leave_second();
    /* Recover before the survivor's shared stage pass: native20E48 treats
     * the hidden passenger's outgoing state3 as a death and starts a retry.
     * Keep its own inventory and continue at the committed script position,
     * with the same native command/door initialization under the new owner. */
    scene_owner=survivor+1;scene_phase=0;
    life.select_frames=life.select_armed=0;
    if(split_views && split_valid) {
        capture(CAMERA,split_cameras[survivor],sizeof saved_camera);split_owner=survivor+1;
    }
    if(survivor)enter_second();
    project(PLAYER,body,sizeof body);
    uint32_t x=le32(leader+8),y=le32(leader+12);
    psx_mod_write_byte(PLAYER+3,1);psx_mod_write_byte(PLAYER+4,1);
    psx_mod_write_byte(PLAYER+0x14,leader[0x14]);psx_mod_write_byte(PLAYER+0x15,leader[0x15]);
    psx_mod_write_byte(PLAYER+0xD0,0);psx_mod_write_byte(PLAYER+0xD4,0);
    psx_mod_write_byte(PLAYER+0x7A,0);psx_mod_write_byte(PLAYER+0x67,0);
    guest(cpu,0x8003A2C8,PLAYER,0);guest(cpu,0x8003A670,PLAYER,0);
    guest(cpu,0x8003CD08,PLAYER,0);
    y=foot_aligned_y(y,leader_box,psx_mod_read_word(PLAYER+0x68));
    psx_mod_write_word(PLAYER+8,x);psx_mod_write_word(PLAYER+12,y);
    psx_mod_write_word(PLAYER+0x18,x);psx_mod_write_word(PLAYER+0x1C,y);
    /* Remove a committed carry's box before native2FB58 computes contacts;
     * clearing it afterward leaves crush bits for the next389E8 HP check. */
    if((int8_t)leader[0xD4]<0)psx_mod_write_word(PLAYER+0x68,0);
    guest(cpu,0x8002FB58,PLAYER,0);
    if(leader[0xD0]) {
        guest(cpu,0x8003D29C,leader[0xD1],leader[0xD2]);
        /* Native fatal entry overwrites F5. A genuinely completed nearby
         * capsule is already phase11 with its program granted; new action18
         * must receive that committed F5=4 return signal. Otherwise37CB4
         * waits forever because phase11 never emits the signal again. */
        if(capsule_return_committed(leader))psx_mod_write_byte(PLAYER+0xF5,4);
    }
    if(leader[0xD4]) {
        /* Native399D4 refuses arrival immunity. Initialize its original
         * action13/D4=FF before granting grace, rather than leaving a normal
         * standing actor inside a moving door. If the old owner committed to
         * the carry,54D24 already removed its box;54D9C restores the new one
         * only at the authored destination. Keep that collisionless phase. */
        psx_mod_write_byte(PLAYER+0xD4,1);psx_mod_write_byte(PLAYER+0xA4,0);
        guest(cpu,0x800399D4,PLAYER,0);
    }
    arrival_grace(cpu);
    if(survivor)leave_second_context(1);
    scene_call=old_scene_call;busy=old_busy;
    psx_mod_counter_add("mmx5.coop.dead-scene-owner-handoffs",1);
}
static int body_on_support(unsigned seat,const uint8_t *body) {
    uint32_t box=le32(body+0x68);
    if(!(body[0x71]&8) || box<0x80010000u || box>0x801FFFFCu)return 0;
    int offset=(int8_t)psx_mod_read_byte(box);
    int x=(int16_t)(le32(body+8)>>16)+(body[0x15]==0x40?-offset:offset);
    int foot=(int16_t)(le32(body+12)>>16)+(int8_t)psx_mod_read_byte(box+1)+psx_mod_read_byte(box+3);
    for(unsigned i=0;i<64;++i) {
        uint32_t actor=0x800C6A10u+i*0xA4u;
        if(!psx_mod_read_byte(actor))continue;
        unsigned phase=psx_mod_read_byte(actor+4);
        /* BIN9's checkpoint support deliberately stays in phase3 (F75F8),
         * while its root F74EC still runs native3240C. Phase2 retires it.
         * Keep the same current contact/carry/geometry checks below; a valid
         * stationary support must not leave the incoming partner suspended. */
        if(phase!=1 && !(phase==3 && psx_mod_read_byte(actor+1)==0x0E &&
           (psx_mod_read_byte(actor+2)&0x80) && psx_mod_read_byte(PLAY+0x0C)==6 &&
           psx_mod_read_byte(PLAY+0x0D)==1 && psx_mod_read_word(0x800EE970u)==13 &&
           psx_mod_read_word(0x800FA480u)==0x800F74ECu &&
           psx_mod_read_word(0x800FB298u)==0x800F75F8u))continue;
        unsigned sides=seat?solid_history[i].sides:psx_mod_read_byte(actor+0x72);
        int carried=(int8_t)(seat?solid_history[i].carried:psx_mod_read_byte(actor+0x76));
        if(!(sides&8) || carried<=0)continue;
        uint32_t solid_box=psx_mod_read_word(actor+0x68);
        if(solid_box<0x80010000u || solid_box>0x801FFFFCu)continue;
        int top=(int16_t)psx_mod_read_half(actor+14)+(int8_t)psx_mod_read_byte(solid_box+1)-psx_mod_read_byte(solid_box+3);
        offset=(int8_t)psx_mod_read_byte(solid_box);
        int center=(int16_t)psx_mod_read_half(actor+10)+(psx_mod_read_byte(actor+0x15)==0x40?-offset:offset);
        int reach=psx_mod_read_byte(box+2)+psx_mod_read_byte(solid_box+2);
        /* Native2ECB0 records solid-top contact in body71/actor72, while
         * terrain uses body70. Require its current integer top equality and
         * strict horizontal overlap as well as this seat's retained carry;
         * stale histories and ordinary airborne actors cannot release a scene
         * or finish a partner arrival on a dynamic platform. */
        if(foot==top && x>center-reach && x<center+reach)return 1;
    }
    return 0;
}
static int mounted_return_anchor(unsigned owner,int locked,const uint8_t *leader,
        Mmx5CoopReturnAnchor *anchor) {
    Mmx5CoopMountedReturn rider={owner,(unsigned)native_armor_overlay(),(unsigned)locked,
        leader[4],leader[5],leader[0x67],leader[0xD5]};
    Mmx5CoopArmorSurface rides[2];
    for(unsigned i=0;i<2;++i) {
        uint32_t actor=0x800D1DC0u+i*0xB0u;
        rides[i]=(Mmx5CoopArmorSurface){psx_mod_read_word(actor+8),psx_mod_read_word(actor+12),
            psx_mod_read_word(actor+0x68),psx_mod_read_byte(actor),psx_mod_read_byte(actor+1),
            psx_mod_read_byte(actor+4),
            (uint8_t)(psx_mod_read_byte(actor+0x70)|psx_mod_read_byte(actor+0x71)),
            psx_mod_read_byte(actor+0x97)};
    }
    return mmx5_coop_armor_return_anchor(&rider,rides,anchor);
}
static int standing_return_position(CPUState *cpu,uint32_t *x,uint32_t *y,
        int spaced,int support) {
    uint8_t body[0x158],collision[0x20];
    capture(PLAYER,body,sizeof body);capture(0x800E9308u,collision,sizeof collision);
    static const int mounted_offsets[]={0,8,-8,16,-16,24,-24,32,-32};
    static const int stage_offsets[]={24,-24,16,-16,32,-32,0};
    static const int capsule_offsets[]={80,-80,72,-72};
    static const int fixed_offsets[]={0};
    const int *offsets=spaced==3?fixed_offsets:spaced==2?capsule_offsets:spaced?stage_offsets:mounted_offsets;
    unsigned count=spaced==3?1:spaced==2?sizeof capsule_offsets/sizeof capsule_offsets[0]:
        spaced?sizeof stage_offsets/sizeof stage_offsets[0]:
        sizeof mounted_offsets/sizeof mounted_offsets[0];
    int safe=0;
    /* A collected capsule's upper lip is above the surrounding floor.
     * Keep its passenger outside the native64-pixel admission rectangle,
     * allowing a bounded drop only when original terrain proves support.
     * Corridor arrows likewise have a step beside their receiving plinth. */
    for(unsigned probe=0;probe<count*(spaced>=2?5u:1u);++probe) {
        unsigned i=probe%count,drop=spaced>=2?(probe/count)*8u:0;
        uint32_t candidate=*x+(uint32_t)(offsets[i]*65536);
        int center=(int16_t)(candidate>>16),camera=(int16_t)psx_mod_read_half(CAMERA+0x0A);
        if(offsets[i] && (center<camera+16 || center>camera+304))continue;
        project(PLAYER,body,sizeof body);
        psx_mod_write_word(PLAYER+8,candidate);psx_mod_write_word(PLAYER+0x18,candidate);
        psx_mod_write_word(PLAYER+12,*y+drop*65536u);psx_mod_write_word(PLAYER+0x1C,*y+drop*65536u);
        psx_mod_write_byte(PLAYER+0x70,0);psx_mod_write_byte(PLAYER+0x71,0);
        psx_mod_write_byte(PLAYER+0x79,0);
        guest(cpu,0x8002FB58u,PLAYER,0);
        unsigned contact=psx_mod_read_byte(PLAYER+0x70)|psx_mod_read_byte(PLAYER+0x71);
        /* A leader's current carrier can support a spaced partner only when
         * the candidate's own feet/box overlap that same live support. This
         * local geometry check writes no guest contact or carrier flags. */
        if(support && !(contact&8u)) {
            uint8_t supported[0x158];capture(PLAYER,supported,sizeof supported);
            supported[0x71]|=8u;
            if(body_on_support((unsigned)(support-1),supported))contact|=8u;
        }
        if(mmx5_coop_return_probe_safe(contact,psx_mod_read_byte(PLAYER+0x79),
                body[0x5C],psx_mod_read_byte(PLAYER+0x5C))) {
            *x=psx_mod_read_word(PLAYER+8);*y=psx_mod_read_word(PLAYER+12);safe=1;break;
        }
    }
    /* Probes do not commit physics, hazard commands or the shared collision
     * scratch. Original incoming animation and world updates still own them. */
    project(PLAYER,body,sizeof body);project(0x800E9308u,collision,sizeof collision);
    return safe;
}
static void down_support_companion_command(unsigned seat,unsigned *command,unsigned *parameter) {
    if(*command!=0x1D || *parameter!=0x40 || scene_phase ||
       life.status[0]!=MMX5_COOP_ALIVE || life.status[1]!=MMX5_COOP_ALIVE ||
       psx_mod_read_byte(PLAY+12)!=8 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=0x0Fu ||
       psx_mod_read_word(0x800F9C78u)!=0x800F4A20u ||
       psx_mod_read_word(0x800F9C8Cu)!=0x800F4AD4u ||
       psx_mod_read_word(0x800F4AE8u)!=0x28420849u ||
       psx_mod_read_word(0x800F4AF8u)!=0x0C00F4A7u)return;
    uint32_t box=psx_mod_read_word(PLAYER+0x68);
    if(box<0x80010000u || box>0x801FFFFCu)return;
    int x=(int16_t)psx_mod_read_half(PLAYER+10);
    int foot=(int16_t)psx_mod_read_half(PLAYER+14)+(int8_t)psx_mod_read_byte(box+1)+
        psx_mod_read_byte(box+3);
    for(uint32_t actor=0x800C6A10u;actor<0x800C9310u;actor+=0xA4u) {
        SolidHistory *h=solid_record(actor);
        if(!h || !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x12 ||
           psx_mod_read_byte(actor+2) || psx_mod_read_byte(actor+4)!=1 ||
           psx_mod_read_byte(actor+5)!=5 || psx_mod_read_byte(actor+6))continue;
        unsigned sides=seat?h->sides:psx_mod_read_byte(actor+0x72);
        int carried=(int8_t)(seat?h->carried:psx_mod_read_byte(actor+0x76));
        uint32_t solid_box=psx_mod_read_word(actor+0x68);
        if(!(sides&8u) || carried<=0 || solid_box!=0x800F9BA4u ||
           psx_mod_read_word(solid_box)!=0x18500800u ||
           foot!=(int16_t)psx_mod_read_half(actor+14)+8-24 ||
           x<=(int16_t)psx_mod_read_half(actor+10)-80-(int)psx_mod_read_byte(box+2) ||
           x>=(int16_t)psx_mod_read_half(actor+10)+80+(int)psx_mod_read_byte(box+2))continue;
        /* F4AD4 stops the owner's original1D walk at world X2121. Mirroring
         * its duration carries a centred partner off the other edge. Give
         * this real rider its own native1D direction/stand14 command toward
         * that same authored target; never move its position or contacts. */
        int32_t delta=(int32_t)psx_mod_read_word(PLAYER+8)-2121*65536;
        if(delta>65536)*parameter=0;
        else if(delta>=-65536)*command=0x14;
        return;
    }
}
static void duel_companion_walk(CPUState *cpu,unsigned passenger) {
    /* Both participants crossed the actual inner door. Keep that living
     * body in the room while the original NPC script supplies its opponent
     * anchor. Native player movement/terrain perform the short entrance;
     * no outgoing beam or second incoming animation is necessary. */
    uint8_t play[sizeof saved_play],camera[sizeof saved_camera],pad[6];
    capture(PLAY,play,sizeof play);capture(CAMERA,camera,sizeof camera);
    capture(PAD,pad,sizeof pad);
    unsigned old_busy=busy,old_call=scene_call;busy=scene_call=1;
    if(passenger)enter_second();
    int x=(int16_t)psx_mod_read_half(PLAYER+10);
    int target=(int16_t)(story_duel.x>>16);
    int anchored=story_duel.phase==2 && story_duel.box;
    uint16_t direction=anchored && x<target-2?0x2000u:
        anchored && x>target+2?0x8000u:0;
    if(psx_mod_read_byte(PLAYER+0xD0))guest(cpu,0x8003D2C4,0,0);
    psx_mod_write_byte(PLAY+0x10,0);psx_mod_write_byte(PLAY+0x1C,0);
    psx_mod_write_half(PAD,direction);psx_mod_write_half(PAD+2,direction);
    psx_mod_write_half(PAD+4,0);
    /* Match the ordinary player pass: native input mapping populates the
     * body's cached controls before35040 consumes them. */
    guest(cpu,0x8003CDB8,0,0);
    guest(cpu,0x80035040,0,0);guest(cpu,0x8002FB58,PLAYER,0);
    x=(int16_t)psx_mod_read_half(PLAYER+10);
    if(anchored && x>=target-2 && x<=target+2 &&
       !psx_mod_read_byte(PLAYER+0x67) &&
       (psx_mod_read_byte(PLAYER+0x70)&8u)) {
        psx_mod_write_byte(PLAYER+0x14,story_duel.facing[0]);
        psx_mod_write_byte(PLAYER+0x15,story_duel.facing[1]);
        guest(cpu,0x8003D29C,0x14,0);
        /* Consume the newly issued stand before phase6 advances only art.
         * Otherwise the last walking animation survives the whole intro. */
        guest(cpu,0x80035040,0,0);
        story_duel.arrived=1;scene_phase=6;
        psx_mod_counter_add("mmx5.coop.duel-walked-arrivals",1);
    }
    if(passenger)leave_second();
    project(PLAY,play,sizeof play);project(CAMERA,camera,sizeof camera);
    project(PAD,pad,sizeof pad);scene_call=old_call;busy=old_busy;
}
static void scene_tick(CPUState *cpu) {
    if(!scene_owner || !enrolled || failed)return;
    scene_recover_owner(cpu);
    unsigned owner=scene_owner-1,passenger=owner^1;
    uint8_t leader[0x158];
    if(owner)memcpy(leader,second.body,sizeof leader);else capture(PLAYER,leader,sizeof leader);
    /* The owner can leave the capsule before its passenger beam finishes.
     * Retain the observed native scene kind across that normal movement. */
    uint32_t capsule=capsule_return_scene(leader);
    if(capsule)scene_capsule=1;
    int locked=leader[0xD0] || leader[0xD4] || leader[0x7A] ||
        psx_mod_read_byte(PLAY+0x10) ||
        (scene_phase && psx_mod_read_byte(PLAY+0x1C));
    if(paired_victory) {
        int final_release=!locked && psx_mod_read_byte(PLAY+0x0C)==12 &&
            !psx_mod_read_byte(PLAY+0x0D) && psx_mod_read_word(0x800EE970u)==0x16u;
        if(final_release) {
            /* The final transition returns to combat instead of the
             * Mavericks' outgoing victory. Native3D29C held each body, so
             * release the companion with resident3D2C4 when the owner has
             * actually unlocked. No later boss/HP predicate may strand it. */
            if(life.status[passenger]==MMX5_COOP_ALIVE) {
                unsigned old_busy=busy;busy=scene_call=1;
                if(passenger)enter_second();
                guest(cpu,0x8003D2C4,0,0);
                if(passenger)leave_second();
                scene_call=0;busy=old_busy;
            }
            paired_victory=2; /* Do not reissue stand on an immediate unlock. */
        }
        paired_victory_tick(cpu,owner,leader);
        if(!locked) {scene_owner=0;if(final_release)paired_victory=0;}
        return;
    }
    /* Terrain/actor damage can mark HP80 before the next player pass changes
     * its lifecycle to DYING. A door may acquire its hold in that interval.
     * Never capture that fatal body for a passenger beam; keep its original
     * death entry instead of materializing it with zero health afterward.
     * Recheck the deferred departure too, before scene_body is captured. */
    if(scene_phase==1 && !both_living(passenger))scene_phase=0;
    if(!scene_phase && locked && duel_scope() && story_duel.phase<=2 &&
       life.status[passenger]==MMX5_COOP_ALIVE) {
        duel_companion_walk(cpu,passenger);return;
    }
    /* A nearby companion may have been hovering or still dropping from the
     * door ledge when a native introduction acquired its camera. Proximity
     * alone cannot safely retain that body's suspended physics. Gather it
     * through the existing native beam instead of ticking an airborne body
     * indefinitely under somebody else's room/script lock. Grizzly's initial
     * camera can still include the grounded upper ledge when stand14 starts;
     * recheck visibility as the native view descends into the actual arena. */
    if(!scene_phase && locked && both_living(passenger) &&
       !(passenger?second.body:first.body)[0xD5] &&
       (!scene_companion_grounded(passenger) || !scene_companion_visible(passenger)))scene_phase=1;
    if(!scene_phase) {
        /* Both-door and nearby introductions retain the visible companion.
         * Its normal player pass is suspended during this shared hold; leaving
         * its last walking frame untouched freezes that pose for the scene.
         * Let the original stand program and animation run for this body,
         * then release only its hold when the actual owner unlocks. */
        const uint8_t *body=passenger?second.body:first.body;
        if(both_living(passenger) && !body[0xD5] &&
           (leader[0xD0] || body[0xD0])) {
            uint8_t play[sizeof saved_play],camera[sizeof saved_camera];
            capture(PLAY,play,sizeof play);capture(CAMERA,camera,sizeof camera);
            unsigned old_busy=busy,old_call=scene_call;busy=scene_call=1;
            if(passenger)enter_second();
            if(locked && leader[0xD0]) {
                unsigned command=leader[0xD1],parameter=leader[0xD2];
                down_support_companion_command(passenger,&command,&parameter);
                /* A native intro can replace a held command without first
                 * releasing D0. Rose changes stand14 to entry1D and back;
                 * retaining only its first command leaves the passenger
                 * standing at the door while the owner enters the arena. */
                if(!psx_mod_read_byte(PLAYER+0xD0) ||
                   psx_mod_read_byte(PLAYER+0xD1)!=command ||
                   psx_mod_read_byte(PLAYER+0xD2)!=parameter)
                    guest(cpu,0x8003D29C,command,parameter);
                guest(cpu,0x80035040,0,0);
                /* Match the ordinary second-body pass's contact refresh.
                 * A scripted walk can leave the entrance ledge; stale 70
                 * floor contact otherwise keeps that unsupported passenger
                 * standing throughout the introduction. */
                guest(cpu,0x8002FB58,PLAYER,0);
            }else if(!locked)guest(cpu,0x8003D2C4,0,0);
            if(passenger)leave_second();
            project(PLAY,play,sizeof play);project(CAMERA,camera,sizeof camera);
            scene_call=old_call;busy=old_busy;
        }
        if(passenger && life.status[1]==MMX5_COOP_LEAVING) {
            unsigned old_busy=busy;busy=1;enter_second();guest(cpu,0x80035EDC,PLAYER,0);leave_second();busy=old_busy;
            if(second.body[4]==3)mmx5_coop_teleport_done(&life);
        }
        if(!locked)scene_owner=0;
        return;
    }
    uint32_t boss=psx_mod_read_word(PLAY+0x20);
    int grizzly_intro=grizzly_stage() && boss>=0x80092090u && boss<0x80093DD0u &&
        !((boss-0x80092090u)%0x9Cu) && psx_mod_read_byte(boss) &&
        psx_mod_read_byte(boss+1)==0x36 && !psx_mod_read_byte(boss+2) &&
        psx_mod_read_byte(boss+4)==1 && psx_mod_read_byte(boss+5)==9;
    int held_arrival=duel_scope() || grizzly_intro;
    if(locked && scene_phase>=4 && !held_arrival)scene_phase=1;
    Mmx5CoopReturnAnchor anchor={le32(leader+8),le32(leader+12),le32(leader+0x68)};
    int arrow_return=scene_corridor_arrow && psx_mod_read_byte(PLAY+12)==2 &&
        psx_mod_read_byte(PLAY+13)==1 && psx_mod_read_word(0x800EE970u)==7 &&
        psx_mod_read_word(0x800F9DE0u)==0x800F26ACu &&
        psx_mod_read_word(0x800F9358u)==0x800F2B98u;
    int armor=0;
    if(scene_phase==3) {
        if((locked && !held_arrival) || leader[4]!=1 || leader[5]<2)return;
        /* Text releases stand14 before the optional native armor grant
         * reacquires action18. Materializing in that gap beams the partner
         * out a second time and captures its unfinished incoming pose.
         * Retain the actual capsule until its original program finishes or
         * the owner leaves it; the native grant/abort remains authoritative. */
        if(capsule && capsule_return_pending(capsule,leader))return;
        /* The corridor owner regains control on its receiving arrow. Wait
         * until it leaves that native admission rectangle before selecting
         * the companion's beam anchor. Otherwise the owner can walk out
         * during the beam and leave its companion on the rearmed arrow. */
        if(arrow_return &&
           corridor_arrow_near((int16_t)(anchor.x>>16),(int16_t)(anchor.y>>16)))return;
        if(duel_scope() && !duel_return_anchor(&anchor))return;
        if(!arrow_return && (leader[0x67] || (!(leader[0x70]&8) && !body_on_support(owner,leader)))) {
            armor=mounted_return_anchor(owner,locked,leader,&anchor);
            if(!armor)return;
        }
    }
    unsigned old_busy=busy;busy=scene_call=1;
    uint8_t play[sizeof saved_play],camera[sizeof saved_camera];
    capture(PLAY,play,sizeof play);capture(CAMERA,camera,sizeof camera);
    if(passenger)enter_second();
    if(scene_phase==1) {
        clear_buster_effects(passenger);
        capture(PLAYER,scene_body,sizeof scene_body);
        for(unsigned i=0;i<sizeof second.shots;++i)psx_mod_write_byte(SHOTS+i,0);
        /* The beam discards this pool and its delayed buster emitters. The
         * saved body must discard their counters too: restoring98==1 with
         * an empty pool blocks Fourth Armor's one-shot full-charge limit
         * until death. Keep9A, the native cyclic slot cursor, unchanged. */
        const unsigned shot_counts[]={0x98,0x99,0xA6};
        for(unsigned i=0;i<sizeof shot_counts/sizeof shot_counts[0];++i) {
            scene_body[shot_counts[i]]=0;psx_mod_write_byte(PLAYER+shot_counts[i],0);
        }
        clear_personal_drawables(passenger);
        guest(cpu,0x8003A2C8,PLAYER,0);guest(cpu,0x8003B33C,PLAYER,3);guest(cpu,0x8003B28C,PLAYER,0);
        psx_mod_write_word(PLAYER+0x54,0);scene_phase=2;
    } else if(scene_phase==2) {
        guest(cpu,0x80035EDC,PLAYER,0);
        if(psx_mod_read_byte(PLAYER+4)==3)scene_phase=3;
    } else if(scene_phase==3) {
        uint8_t hidden[0x158];capture(PLAYER,hidden,sizeof hidden);
        project(PLAYER,scene_body,sizeof scene_body);
        /* Hover/jump collision suspension belongs to the departed pose.
         * Materialization returns an on-foot actor at a grounded anchor. */
        if(!psx_mod_read_byte(PLAYER+0xD5))psx_mod_write_byte(PLAYER+0x67,0);
        guest(cpu,0x8003CD08,PLAYER,0);
        uint32_t x=anchor.x,landing=foot_aligned_y(anchor.y,anchor.box,psx_mod_read_word(PLAYER+0x68));
        if(arrow_return) {
            /* Use the admitted native source pair, not the owner's first
             * landing after the fade. Both corridor floors are authored;
             * place the companion outside the receiving trigger and prove
             * its own standing box has safe terrain before materializing. */
            unsigned index=scene_corridor_arrow-1;
            uint32_t box=psx_mod_read_word(PLAYER+0x68);
            if(box<0x80010000u || box>0x801FFFFCu) {
                project(PLAYER,hidden,sizeof hidden);goto scene_done;
            }
            int foot=(int8_t)psx_mod_read_byte(box+1)+psx_mod_read_byte(box+3);
            x=(uint32_t)(((int16_t)psx_mod_read_half(0x800F9D0Cu+index*4)+40)*65536);
            landing=(uint32_t)(((index&1?416:896)-foot)*65536);
            if(!standing_return_position(cpu,&x,&landing,3,0)) {
                project(PLAYER,hidden,sizeof hidden);goto scene_done;
            }
        }
        psx_mod_write_byte(PLAYER+0x14,duel_scope()?story_duel.facing[0]:leader[0x14]);
        psx_mod_write_byte(PLAYER+0x15,duel_scope()?story_duel.facing[1]:leader[0x15]);
        if(armor && !standing_return_position(cpu,&x,&landing,0,0)) {
            project(PLAYER,hidden,sizeof hidden);goto scene_done;
        }
        if(psx_mod_read_byte(PLAY+12)==18 && !psx_mod_read_byte(PLAY+13) &&
           capsule_return_committed(leader)) {
            /* Return beside the collected secret capsule when its real
             * floor permits. Centering the other recipient on the owner
             * makes the rearmed capsule open its next scene around them. */
            (void)standing_return_position(cpu,&x,&landing,2,(int)owner+1);
        }
        scene_y=(int32_t)landing;
        uint32_t y=(uint32_t)(((int16_t)psx_mod_read_half(CAMERA+0x0E)-40)*65536);
        psx_mod_write_word(PLAYER+8,x);psx_mod_write_word(PLAYER+12,y);
        psx_mod_write_word(PLAYER+0x18,x);psx_mod_write_word(PLAYER+0x1C,y);
        psx_mod_write_byte(PLAYER+3,1);psx_mod_write_byte(PLAYER+4,1);
        psx_mod_write_byte(PLAYER+0x14,duel_scope()?story_duel.facing[0]:leader[0x14]);
        psx_mod_write_byte(PLAYER+0x15,duel_scope()?story_duel.facing[1]:leader[0x15]);
        guest(cpu,0x8003A2C8,PLAYER,0);guest(cpu,0x8003B33C,PLAYER,1);
        psx_mod_write_word(PLAYER+0x68,0);psx_mod_write_word(PLAYER+0x54,0);scene_phase=4;
        psx_mod_counter_add("mmx5.coop.scene-incoming-starts",1);
    } else if(scene_phase==6) {
        /* The companion has arrived during the original dialogue. Keep its
         * native hold until the owner releases control; never beam twice. */
        if(duel_scope())guest(cpu,0x80017248,PLAYER,0);
        else {
            guest(cpu,0x80035040,0,0);
            guest(cpu,0x8002FB58,PLAYER,0);
        }
        if(!locked && (!duel_scope() || story_duel.intro_complete)) {
            guest(cpu,0x8003D2C4,0,0);
            scene_owner=scene_phase=0;
            if(duel_scope())story_duel.phase=3;
        }
    } else {
        guest(cpu,0x80017248,PLAYER,0);
        if(scene_phase==4) {
            int32_t y=(int32_t)psx_mod_read_word(PLAYER+12)+8*65536;
            if(y>=scene_y) {y=scene_y;guest(cpu,0x8003B33C,PLAYER,2);scene_phase=5;}
            psx_mod_write_word(PLAYER+12,(uint32_t)y);
        } else if(!psx_mod_read_byte(PLAYER+0x46)) {
            if(!held_arrival)guest(cpu,0x8003D2C4,0,0);
            guest(cpu,0x8003A670,PLAYER,0);guest(cpu,0x8003CD08,PLAYER,0);
            guest(cpu,0x8002FB58,PLAYER,0);
            if(duel_scope()) {
                story_duel.arrived=1;scene_phase=6;
            } else if(grizzly_intro) {
                guest(cpu,0x8003D29C,leader[0xD1],leader[0xD2]);scene_phase=6;
            } else {
                /* A healthy shared-scene passenger has landed through the
                 * native beam. Do not add a fictitious injury/blink interval. */
                scene_owner=scene_phase=0;
            }
        }
    }
scene_done:
    if(passenger)leave_second();
    project(PLAY,play,sizeof play);project(CAMERA,camera,sizeof camera);
    scene_call=0;busy=old_busy;
}
/* These accesses follow the physical PLAYER projection, including a P2-owned
 * door inside a P1 world pass. world_owner alone is not sufficient here. */
static uint8_t both_body_byte(unsigned seat,unsigned at) {
    return seat==projected?psx_mod_read_byte(PLAYER+at):(seat?second.body:first.body)[at];
}
static uint32_t both_body_word(unsigned seat,unsigned at) {
    return seat==projected?psx_mod_read_word(PLAYER+at):le32((seat?second.body:first.body)+at);
}
static void both_body_word_write(unsigned seat,unsigned at,uint32_t value) {
    if(seat==projected)psx_mod_write_word(PLAYER+at,value);
    else put32((seat?second.body:first.body)+at,value);
}
static void both_gate(unsigned seat,uint8_t value) {
    if(seat==projected)psx_mod_write_byte(PLAYER+0xD4,value);
    else (seat?second.body:first.body)[0xD4]=value;
}
static int both_living(unsigned seat) {
    return life.status[seat]==MMX5_COOP_ALIVE && both_body_byte(seat,4)==1 &&
        (both_body_byte(seat,0x5C)&0x7F);
}
static void water_head_round(CPUState *cpu,uint32_t address) {
    (void)address;
    if(busy || !enrolled || failed || psx_mod_local_view_scope() ||
       (projected && !world_owner) || cpu->gpr[17]!=PLAYER || !water_head_stage() ||
       psx_mod_read_half(0x80102FD4u) || psx_mod_read_byte(cpu->gpr[16]+0x1D))return;
    unsigned seat=1u-(unsigned)(projected!=0);
    unsigned action=both_body_byte(seat,5);
    if(!both_living(seat) || both_body_byte(seat,0xCC) || action<2 || action==0x11)return;
    /* EECA8 follows the canonical allocation and precedes the one RNG
     * reload. The gate/expired clock distinguish an admitted round from
     * its gated reload path. Preserve P1's last-slot preference, then add
     * the other eligible head without replaying the controller or clock. */
    uint32_t actor=guest(cpu,0x8002D27Cu,0,0);LandingEffect *effect=landing_effect(actor);
    if(!effect)return;
    effect->owner=(uint8_t)(seat+1);
    psx_mod_write_byte(actor,0x41);psx_mod_write_byte(actor+1,0x18);
    psx_mod_write_byte(actor+2,0x0B);
    psx_mod_counter_add("mmx5.coop.paired-water-head-rounds",1);
}
static void water_head_origin(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[17];LandingEffect *effect=landing_effect(actor);
    if(!enrolled || failed || psx_mod_local_view_scope() || !water_head_stage() ||
       !effect || !effect->owner || effect->owner>2 || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=0x18 || psx_mod_read_byte(actor+2)!=0x0B ||
       psx_mod_read_byte(actor+4))return;
    unsigned seat=effect->owner-1;
    /* NativeF7F3C reads only integer head XY/facing before its original
     * animation/rise. Replace those loaded values, keeping PLAYER, stage
     * palettes, art, shared clocks and PGXP memory shadows untouched. */
    if(address==0x800F7F4Cu)cpu->gpr[3]=(uint16_t)(both_body_word(seat,12)>>16);
    else if(address==0x800F7F5Cu)cpu->gpr[2]=both_body_byte(seat,0x15);
    else cpu->gpr[2]=(uint16_t)(both_body_word(seat,8)>>16);
}
static void both_half(uint8_t *p,unsigned at,int value) {
    p[at]=(uint8_t)value;p[at+1]=(uint8_t)((unsigned)value>>8);
}
static void both_camera_bounds(int minimum,int maximum) {
    psx_mod_write_half(CAMERA+0x26,(uint16_t)minimum);
    psx_mod_write_half(CAMERA+0x24,(uint16_t)maximum);
    if(split_views && split_valid)for(unsigned seat=0;seat<2;++seat) {
        both_half(split_cameras[seat],0x26,minimum);both_half(split_cameras[seat],0x24,maximum);
    }
}
static void both_project(unsigned seat) {
    if(seat && !projected)enter_second();
    else if(!seat && projected)leave_second_context(1);
}
static void both_native_entry(CPUState *cpu,unsigned seat) {
    if(!both_living(seat))return;
    /* The entrant's ordinary player pass consumes the positive door gate
     * through native399D4, selecting its scripted walk/dash animation before
     * the shared hold. Writing FF directly for the companion skipped that
     * initialization. Ticking an ended transient's timer then left its last
     * sprite unchanged throughout the carry. Initialize the native action;
     * the shared door remains the sole movement and scene-clock owner. */
    unsigned was=projected,old_busy=busy;busy=1;
    both_project(seat);psx_mod_write_byte(PLAYER+0xD4,1);
    /* The shared entry is already committed. Unlike P1, this suspended body
     * cannot wait out A4 and retry initialization on its next ordinary pass. */
    uint8_t immunity=psx_mod_read_byte(PLAYER+0xA4);
    psx_mod_write_byte(PLAYER+0xA4,0);
    guest(cpu,0x800399D4,PLAYER,0);
    psx_mod_write_byte(PLAYER+0xA4,immunity);
    both_project(was);busy=old_busy;
    psx_mod_counter_add("mmx5.coop.both-door-native-companion-entries",1);
}
static void both_restore_control(CPUState *cpu,unsigned seat) {
    if(!both_living(seat) || both_body_byte(seat,0xD5))return;
    unsigned was=projected,old_busy=busy;busy=1;
    both_project(seat);
    clear_buster_effects(seat);clear_personal_drawables(seat);
    if(both_door_native_carry() &&
       !both_body_byte(both_door.owner,0xD0) && psx_mod_read_byte(PLAYER+0xD0)) {
        /* The shared native carry releases its entrant's stand command.
         * The separately initialized companion must consume that release
         * too; otherwise its action14/D0=FF survives the door indefinitely,
         * even after its own gate and the presentation lock have cleared. */
        uint8_t lock=psx_mod_read_byte(PLAY+0x1C);
        guest(cpu,0x8003D2C4,0,0);
        psx_mod_write_byte(PLAY+0x1C,lock);
    }
    if(both_door_native_carry() && psx_mod_read_byte(PLAYER+5)==0x13) {
        /* Native action13 owns its exit too. With D4 released,37888 starts
         * its fall/landing program;3797C advances gravity until real contact.
         * Replacing that program with idle2 strands an airborne body's67
         * suspension flag. Restore its own box, then retain native control
         * recovery instead of manufacturing a grounded standing pose. */
        uint8_t pad[6];capture(PAD,pad,sizeof pad);
        /* Native378C8 can immediately call the live input dispatcher when
         * the restored body is grounded. This callback runs in the door's
         * world scope, where PAD belongs to the entrant. Keep its locked
         * recovery neutral instead of lending those buttons to the partner. */
        for(unsigned i=0;i<sizeof pad;++i)psx_mod_write_byte(PAD+i,0);
        guest(cpu,0x8003CD08,PLAYER,0);guest(cpu,0x80037888,PLAYER,0);
        project(PAD,pad,sizeof pad);
    } else {
        guest(cpu,0x8003A2C8,PLAYER,0);guest(cpu,0x8003A670,PLAYER,0);
        guest(cpu,0x8003CD08,PLAYER,0);
    }
    both_project(was);
    busy=old_busy;
}
static int training_boss_actor(uint32_t actor) {
    /* Training's native Magma Dragoon uses class30 rather than a campaign
     * boss class. Its initializer issues the same stand14 command before
     * the original health fill and control release. Match the loaded module
     * and its original dispatch, since class30 has other stage meanings. */
    return psx_mod_read_byte(PLAY+12)==22 && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_byte(actor+1)==0x30 && !psx_mod_read_byte(actor+2) &&
        psx_mod_read_word(0x800EE970u)==0x21u &&
        psx_mod_read_word(0x800F8B38u)==0x800F4010u &&
        psx_mod_read_word(0x800FA3ECu)==0x800F3444u &&
        psx_mod_read_word(0x800F3558u)==0x0C00F4A7u;
}
static int both_door_pending_intro(void) {
    unsigned stage=psx_mod_read_byte(PLAY+12);
    for(uint32_t actor=0x80092090u;actor<0x80093DD0u;actor+=0x9Cu) {
        if(!psx_mod_read_byte(actor) || psx_mod_read_byte(actor+4))continue;
        unsigned kind=psx_mod_read_byte(actor+1);
        if(training_boss_actor(actor) || (!stage && kind==0x31u) ||
           (stage>=1 && stage<=8 && kind>=0x32u && kind<=0x39u) ||
           (stage==16 && kind==0x3Au) ||
           (stage==17 && kind==0x3Cu) ||
           (stage==18 && (kind==0x40u || kind==0x41u)) || (stage==12 && kind==0x42u))return 1;
    }
    return 0;
}
static void both_door_maintain(CPUState *cpu) {
    if(!both_door.phase || psx_mod_local_view_scope())return;
    if(!both_door_scope() || psx_mod_read_byte(PLAY)!=10) {
        memset(&both_door,0,sizeof both_door);return;
    }
    if(both_door.phase==4) {
        /* An outer passage has no boss introduction to consume phase4.
         * Keep airborne recovery held for an admitted pending boss, but
         * release an ordinary passage when its original lock has ended. */
        if(!scene_owner && !psx_mod_read_byte(PLAY+0x1C) && !both_door_pending_intro())
            memset(&both_door,0,sizeof both_door);
        return;
    }
    uint32_t actor=both_door.actor;
    if(!psx_mod_read_byte(actor) || psx_mod_read_byte(actor+4)!=1 ||
       !psx_mod_read_byte(actor+5) || (!both_living(0) && !both_living(1))) {
        for(unsigned seat=0;seat<2;++seat) {
            if(both_body_byte(seat,0xD4))both_restore_control(cpu,seat);
            both_gate(seat,0);
        }
        if(both_door.owns_lock)psx_mod_write_byte(PLAY+0x1C,0);
        both_camera_bounds(both_door.old_min,both_door.old_max);
        memset(&both_door,0,sizeof both_door);
        psx_mod_counter_add("mmx5.coop.both-door-aborts",1);return;
    }
    if(!both_living(both_door.owner) && both_living(both_door.owner^1u)) {
        both_gate(both_door.owner,0);both_door.owner^=1;
        both_gate(both_door.owner,255);
        psx_mod_counter_add("mmx5.coop.both-door-owner-handoffs",1);
    }
}
static int both_door_query(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!door_call || !door_wait || psx_mod_local_view_scope())return 0;
    psx_mod_counter_add("mmx5.coop.both-door-admission-waits",1);
    cpu->gpr[2]=0;return 1;
}
static int both_door_cleared(void) {
    for(unsigned seat=0;seat<2;++seat)if(both_living(seat)) {
        uint32_t box=both_body_word(seat,0x68);
        if(box<0x80010000u || box>0x801FFFFCu)return 0;
        int x=(int16_t)(both_body_word(seat,8)>>16);
        int left=x+(int8_t)psx_mod_read_byte(box)-psx_mod_read_byte(box+2);
        if(left<=both_door.right || x<both_door.target_min+8)return 0;
    }
    return 1;
}
static void both_companion_animation(CPUState *cpu) {
    if(!both_living(1) || both_body_byte(1,0x67) ||
       (!psx_mod_read_byte(PLAY+0x10) && !psx_mod_read_byte(PLAY+0x1C)))return;
    unsigned was=projected,old_busy=busy;busy=1;both_project(1);
    guest(cpu,0x80017248,PLAYER,0);
    if((int8_t)psx_mod_read_byte(PLAYER+0x46)<0) {
        guest(cpu,0x8003B33C,PLAYER,0x26);
        guest(cpu,0x80017248,PLAYER,0);
    }
    if(!was)leave_second();
    busy=old_busy;
    psx_mod_counter_add("mmx5.coop.both-door-companion-animation-ticks",1);
}
static int both_door_traversal(CPUState *cpu,uint32_t address) {
    if(door_traversal_call || psx_mod_local_view_scope() || !both_door_active() ||
       both_door.actor!=cpu->gpr[4] || both_door.phase<2)return 0;
    if(both_door_native_carry()) {
        /* Run the original once-per-world carry, including its layer1
         * offset, closing event and camera arrival test. Transport only the
         * living companion by the same actual displacement; never synthesize
         * a room/cutscene completion or replay the shared door controller. */
        unsigned companion=projected^1u;
        uint32_t before=psx_mod_read_word(PLAYER+8);
        door_traversal_call=1;
        uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        uint32_t delta=psx_mod_read_word(PLAYER+8)-before;
        if(both_living(companion)) {
            uint8_t *body=companion?second.body:first.body;
            uint32_t x=le32(body+8)+delta,history=le32(body+0x18)+delta;
            for(unsigned i=0;i<4;++i) {
                body[8+i]=(uint8_t)(x>>(i*8));body[0x18+i]=(uint8_t)(history>>(i*8));
            }
            /* The ordinary P1 pass still advances its door animation, while
             * second_tick skips P2 under this shared lock. Advance only that
             * suspended body's native animation, retaining its walk/dash
             * program. A braking/landing program can finish after admission;
             * switch that ended transient to native399D4's authored entry
             * loop26 instead of decrementing its terminal timer forever.
             * The original door remains the sole movement/clock owner. */
            if(companion==1)both_companion_animation(cpu);
        }
        if(psx_mod_read_byte(both_door.actor+5)==4) {
            both_door.phase=4;both_gate(companion,0);
            if(both_living(companion)) {
                /* Recovery may immediately run gravity/contact. Restore only
                 * the native collision box first, then clear the closing
                 * plane before releasing the suspended companion's program. */
                unsigned was=projected,old_busy=busy;busy=1;
                both_project(companion);guest(cpu,0x8003CD08,PLAYER,0);
                both_project(was);busy=old_busy;
                uint32_t box=both_body_word(companion,0x68);
                int32_t x=(int32_t)both_body_word(companion,8);
                int minimum=both_door.target_min+8;
                if(box>=0x80010000u && box<=0x801FFFFCu) {
                    int clear=both_door.right-(int8_t)psx_mod_read_byte(box)+
                        psx_mod_read_byte(box+2)+1;
                    if(clear>minimum)minimum=clear;
                }
                /* A backward dash/contact before the hold may leave the
                 * companion behind the closing plane even after the same
                 * carry delta. Correct only the missing collision clearance;
                 * preserve feet, pose and the rest of the pair's spacing. */
                if((int16_t)(x>>16)<minimum) {
                    uint32_t anchor=(uint32_t)(minimum*65536),shift=anchor-(uint32_t)x;
                    both_body_word_write(companion,8,anchor);
                    both_body_word_write(companion,0x18,both_body_word(companion,0x18)+shift);
                    psx_mod_counter_add("mmx5.coop.both-door-safe-arrivals",1);
                }
            }
            both_restore_control(cpu,companion);
            psx_mod_counter_add("mmx5.coop.both-door-crossings",1);
        }
        door_traversal_call=0;cpu->gpr[2]=result;return 1;
    }
    int x=(int16_t)psx_mod_read_half(CAMERA+0x0A);
    if(!both_door_cleared() || x<both_door.target_min || x>both_door.target_max) {
        cpu->gpr[2]=0;return 1;
    }
    /* Free movement replaces the singleton auto-carry. Once real clearance
     * and camera arrival are established, let the original closing branch
     * own its event/checkpoint effects. The temporary comparison coordinates
     * never move either actor and are restored before the world continues. */
    door_traversal_call=1;
    uint32_t delta=psx_mod_read_word(both_door.actor+0x84);
    psx_mod_write_word(both_door.actor+0x84,0);
    both_camera_bounds(both_door.target_min,both_door.target_max);
    psx_mod_write_half(CAMERA+0x0A,(uint16_t)both_door.target_min);
    both_gate(0,0);both_gate(1,0);both_door.phase=3;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    psx_mod_write_word(both_door.actor+0x84,delta);
    psx_mod_write_half(CAMERA+0x0A,(uint16_t)x);
    door_traversal_call=0;
    if(psx_mod_read_byte(both_door.actor+5)==4) {
        both_door.phase=4;psx_mod_counter_add("mmx5.coop.both-door-crossings",1);
    }
    cpu->gpr[2]=result;return 1;
}
static int both_door_map(CPUState *cpu,uint32_t address) {
    if(door_terrain_call || psx_mod_local_view_scope() || !both_door_scope() ||
       both_door.phase<2 || both_door.phase>3 || cpu->gpr[4]!=PLAYER)return 0;
    uint32_t actor=both_door.actor,box=psx_mod_read_word(actor+0x68);
    if(psx_mod_read_byte(actor+1)!=8 || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+5)!=3 || box<0x80010000u || box>0x801FFFFCu)return 0;
    int x=(int16_t)cpu->gpr[5],y=(int16_t)cpu->gpr[6];
    int cx=(int16_t)psx_mod_read_half(actor+10)+(int8_t)psx_mod_read_byte(box);
    int cy=(int16_t)psx_mod_read_half(actor+14)+(int8_t)psx_mod_read_byte(box+1);
    int w=psx_mod_read_byte(box+2),h=psx_mod_read_byte(box+3);
    if(x<cx-w || x>=cx+w || y<cy-h || y>=cy+h)return 0;
    /* Actual Grizzly RAM has38 wall cells at9344..9375/y32..95 inside
     * this opened aperture; its floor at96 is also38 and stays solid.
     * Keep the original query's sample-coordinate side effects and all
     * other collision classes, including lethal terrain. */
    door_terrain_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    door_terrain_call=0;
    if(result==0x38u) {
        result=0;psx_mod_counter_add("mmx5.coop.both-door-map-passages",1);
    }
    cpu->gpr[2]=result;return 1;
}
static int door_tick_canonical(CPUState *cpu,uint32_t address) {
    if(door_call || !enrolled || failed || psx_mod_local_view_scope() || (projected && !world_owner))return 0;
    door_call=1;uint32_t actor=cpu->gpr[4],result;
    unsigned seat=world_owner,was=projected;
    int paired=both_door_players && !car_context() && !scene_owner &&
        psx_mod_read_byte(actor+4)==1 && !psx_mod_read_byte(actor+5) &&
        both_living(0) && both_living(1);
    unsigned first_hit=0,second_hit=0;
    if(both_door_active() && both_door.actor==actor)seat=both_door.owner;
    else if(scene_owner)seat=scene_owner-1;
    else if(paired) {
        both_project(0);
        if(both_body_word(0,0x68))first_hit=guest(cpu,0x800549E8,actor,0);
        both_project(1);
        if(both_body_word(1,0x68))second_hit=guest(cpu,0x800549E8,actor,0);
        both_project(was);
        if(!first_hit && second_hit)seat=1;
    }
    else if(!world_owner && life.status[1]==MMX5_COOP_ALIVE) {
        first_hit=life.status[0]==MMX5_COOP_ALIVE?
            guest(cpu,0x800549E8,actor,0):0;
        if(!first_hit) {
            enter_second();second_hit=guest(cpu,0x800549E8,actor,0);leave_second();
            if(second_hit)seat=1;
        }
    }
    door_wait=paired && (!first_hit || !second_hit);
    uint8_t canonical_camera[0xFC];unsigned camera_owner=split_owner;
    int scoped_camera=split_views && split_valid && !scene_owner && seat!=world_owner;
    if(scoped_camera) {
        capture(CAMERA,canonical_camera,sizeof canonical_camera);
        project(CAMERA,split_cameras[seat],sizeof canonical_camera);split_owner=seat+1;
    }
    both_project(seat);
    unsigned old_gate=psx_mod_read_byte(PLAYER+0xD4),old_lock=psx_mod_read_byte(PLAY+0x1C);
    int old_min=(int16_t)psx_mod_read_half(CAMERA+0x26),old_max=(int16_t)psx_mod_read_half(CAMERA+0x24);
    result=guest(cpu,address,actor,cpu->gpr[5]);
    if(paired && !door_wait && !old_gate && psx_mod_read_byte(PLAYER+0xD4)) {
        duel_begin(cpu,actor,seat);
        uint32_t box=psx_mod_read_word(actor+0x68);
        int right=(int16_t)psx_mod_read_half(actor+10);
        if(box)right+=(int8_t)psx_mod_read_byte(box)+psx_mod_read_byte(box+2);
        both_door=(BothDoor){actor,(int16_t)old_min,(int16_t)old_max,(int16_t)old_min,
            (int16_t)old_max,(int16_t)right,psx_mod_read_byte(PLAY+0x0C),
            psx_mod_read_byte(PLAY+0x0D),1,(uint8_t)seat,
            (uint8_t)(!old_lock && psx_mod_read_byte(PLAY+0x1C))};
        both_native_entry(cpu,seat^1u);
        both_gate(seat^1u,255);life.select_frames=life.select_armed=0;
        psx_mod_counter_add("mmx5.coop.both-door-starts",1);
    }
    if(both_door_active() && both_door.actor==actor) {
        /* P2's ordinary pass is already suspended during opening, before
         * native54D9C begins the carry. Consume its new entry program now;
         * otherwise its old standing sprite persists until walking starts.
         * The carry advances the same body once through its own callback. */
        if(both_door.phase==1 && both_door_native_carry())both_companion_animation(cpu);
        /* Original54D24 removes the entrant's collision box throughout its
         * scripted opening/carry. Match that suspension for the companion:
         * leaving its box live lets the roof conveyor keep dragging it
         * backward while its ordinary player pass is held. Native restore
         * and the existing abort path reinstate each character's own box. */
        if(both_door_native_carry() && psx_mod_read_byte(actor+5)>=2 &&
           !psx_mod_read_word(PLAYER+0x68) && both_living(seat^1u))
            both_body_word_write(seat^1u,0x68,0);
        if(both_door.phase==1 && psx_mod_read_byte(actor+5)==3) {
            both_door.target_min=(int16_t)psx_mod_read_half(CAMERA+0x26);
            both_door.target_max=(int16_t)psx_mod_read_half(CAMERA+0x24);
            if(!both_door_native_carry()) {
                both_camera_bounds(both_door.old_min,both_door.target_max);
                both_restore_control(cpu,seat);both_gate(seat,255);
                if(both_door.owns_lock)psx_mod_write_byte(PLAY+0x1C,0);
            }
            both_door.phase=2;
            psx_mod_counter_add(both_door_native_carry()?"mmx5.coop.both-door-native-carries":
                                                       "mmx5.coop.both-door-free-controls",1);
        }
    } else if(psx_mod_read_byte(PLAYER+0xD4)) {
        if(!old_gate)duel_begin(cpu,actor,seat);
        scene_begin(seat);
    }
    door_wait=0;
    both_project(was);
    if(both_door_scope() && both_door.actor==actor && both_door.phase>=2) {
        int minimum=both_door.phase<4 && !both_door_native_carry()?both_door.old_min:both_door.target_min;
        both_half(canonical_camera,0x26,minimum);both_half(canonical_camera,0x24,both_door.target_max);
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
#include "mmx5_coop_duel.inc"
static void team_bounds(void) {
    if(!enrolled || failed || scene_owner || psx_mod_read_byte(PLAY)!=10 ||
       psx_mod_read_byte(PLAY+0x10) || psx_mod_read_byte(PLAY+0x1C))return;
    /* BIN4's authored whale rooms use their original per-player bounds,
     * already applied to both heroes by whale_player_bounds. CAMERA+20 can
     * still hold the checkpoint's old bottom while the scripted view drops;
     * treating it as an ordinary pit floor kills a healthy hero at Y504.
     * Leave those rooms to their native clamps/crush rules. Hallway phase4
     * and the final room retain the ordinary bounds below. */
    if(whale_authored_camera())return;
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
static void intro_climb_camera(void) {
    /* Native area record7C798 retires after locking the roof's bottom192.
     * Co-op delays the closing structures until the last living climber,
     * so that one-way lock can no longer assume the return path is closed.
     * Reopen only this actual vertical corridor when its resident descends;
     * retain the authored roof lock, other rooms and real pit deaths. */
    if(!split_views || !enrolled || failed || scene_owner || both_door_active() ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12) || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=4u ||
       psx_mod_read_word(0x8007C798u)!=0x117F1040u ||
       psx_mod_read_word(0x8007C7A4u)!=0x00C0FFFFu ||
       psx_mod_read_byte(PLAYER+4)!=1 || !(psx_mod_read_byte(PLAYER+0x5C)&127u))return;
    int x=(int16_t)psx_mod_read_half(PLAYER+10),y=(int16_t)psx_mod_read_half(PLAYER+14);
    if(x<4160 || x>=4480 || psx_mod_read_half(CAMERA+0x1C)!=4160 ||
       psx_mod_read_half(CAMERA+0x1E)!=4160)return;
    if(y>383) {
        psx_mod_write_half(CAMERA+0x20,960);psx_mod_write_half(CAMERA+0x28,960);
    } else if(y<=363)psx_mod_write_half(CAMERA+0x28,192);
}
static int camera_update(CPUState *cpu,uint32_t address) {
    if(camera_update_call || split_camera_call || psx_mod_local_view_scope() || busy || !enrolled || failed ||
       psx_mod_read_byte(PLAY)!=10)return 0;
    if(split_views && split_valid && split_owner!=world_owner+1 && !scene_owner && !car_context())
        project(CAMERA,split_cameras[world_owner],sizeof saved_camera);
    camera_update_call=1;team_bounds();intro_climb_camera();rose_private_camera_area();
    dinorex_private_camera_focus();
    uint32_t result;
    int late_scene_owner=split_views && split_valid && !projected && !world_owner &&
        scene_owner==2 && scene_phase && scene_passenger(0) &&
        life.status[0]==MMX5_COOP_ALIVE && life.status[1]==MMX5_COOP_ALIVE &&
        second.body[4]==1 && !car_context() &&
        psx_mod_read_byte(PLAY)==10 &&
        psx_mod_read_word(0x8002A708u)==0x0C00A461u &&
        psx_mod_read_word(0x800292CCu)==0xA0A2005Cu;
    if(late_scene_owner) {
        /* A P2 scene admission can acquire scene_begin's complete camera
         * during this still-running P1 world pass. The native foreground
         * update then calls29184 before stage_tick can select the new owner
         * on the next pass. Its PLAYER must belong to that admitted camera:
         * otherwise the waiting passenger is clamped/killed by roof bounds
         * before its outgoing beam can finish. Keep the actual camera and
         * original fatal rules; project only the living scene owner's body.
         * Shared quake/follow code still executes once at its native site. */
        uint8_t campaign=psx_mod_read_byte(PLAY+0x44);
        enter_second();world_owner=1;
        psx_mod_write_byte(PLAY+0x44,campaign);
        psx_mod_write_word(SCRATCH+0x28,resources[3]);
        result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        world_owner=0;leave_second_context(1);
        psx_mod_counter_add("mmx5.coop.late-scene-camera-projections",1);
    } else result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    intro_private_layer_region();
    rose_private_layer_region();
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
static int both_door_pending_boss(uint32_t actor) {
    unsigned stage=psx_mod_read_byte(PLAY+0x0C);
    /* Native Maverick initialization issues stand14 immediately. With free
     * Both-door traversal that would stop the first hero before the other
     * clears the aperture (stage8's drop).
     * Keep the original uninitialized actor until the door has closed and
     * the living pair has landed. An immediate stand14 otherwise interrupts
     * action13's native fall, holding its owner in the air and beaming the
     * passenger before it can reach the lower arena floor. The original
     * initializer still owns the warning, HP fill and control return. */
    int landing=both_door_scope() && both_door.phase==4 &&
        both_living(0) && both_living(1) &&
        (!scene_companion_grounded(0) || !scene_companion_grounded(1));
    if((!both_door_active() && !landing) || actor<0x80092090u || actor>=0x80093DD0u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+4))return 0;
    unsigned kind=psx_mod_read_byte(actor+1),subtype=psx_mod_read_byte(actor+2);
    if(training_boss_actor(actor))return 1;
    /* BIN0's Sigma initializer also issues stand14. With Both doors its
     * placement is admitted before either body reaches the final clearance
     * at4552, freezing the door owner at4536 and preventing native closure.
     * Defer only original class31 initialization, like the Mavericks. */
    if(!stage && kind==0x31u && !subtype &&
       psx_mod_read_word(0x800EE970u)==4u &&
       psx_mod_read_word(0x800FB4A4u)==0x800F7FC4u &&
       psx_mod_read_word(0x800F8084u)==0x0C00F4A7u)return 1;
    if(stage>=1 && stage<=8)return !subtype && kind>=0x32u && kind<=0x39u;
    /* Shadow Devil also issues stand14 before its original assembly.
     * Starting it inside the paired carry loses that command when the door
     * restores control, leaving its introduction waiting on the wrong pose. */
    if(stage==16 && kind==0x3Au && !subtype &&
       psx_mod_read_word(0x800EE970u)==0x1Eu &&
       psx_mod_read_word(0x800F8D5Cu)==0x800F3D54u &&
       psx_mod_read_word(0x800F22F8u)==0x0C00F4A7u)return 1;
    /* Rangda Bangda also begins with stand14. Admit that native shared
     * introduction after the crossing, so it acquires and releases both
     * heroes instead of stranding the projected companion in stand14. */
    if(stage==17 && kind==0x3Cu && !subtype &&
       psx_mod_read_word(0x800EE970u)==0x1Fu &&
       psx_mod_read_word(0x800FDA90u)==0x800F9CF0u &&
       psx_mod_read_word(0x800F9D08u)==0x0C00F4A7u &&
       psx_mod_read_word(0x800FA104u)==0x0C00F4B1u)return 1;
    /* The raw12 final body can load while Both traversal is still open.
     * Its native FB9C8 stand14 would then miss scene_begin's admission and
     * lock only the selected hero. Keep this original body uninitialized
     * until both heroes commit the crossing, just like the Mavericks above.
     * Match the stage and loaded BIN38 dispatch; FA000 is shared by other
     * dynamic modules, including the later final phase. */
    if(stage==12 && kind==0x42u && !subtype &&
       psx_mod_read_word(0x800EE970u)==0x16u &&
       psx_mod_read_word(0x800FA000u)==0x2Au &&
       psx_mod_read_word(0x800FD888u)==0x800FA024u &&
       psx_mod_read_word(0x800FA024u)==0x27BDFFE0u &&
       psx_mod_read_word(0x800FB424u)==0x27BDFFE8u)return 1;
    /* BIN28's story opponents issue stand14 during initialization. Keep
     * native setup/text after the physical crossing, using the canonical
     * campaign even while P2 is projected. Wrong-campaign actors must still
     * run their original early retirement. F8BC0 selects class41 subtype0/1
     * from the native story branch; both share the same initializer. */
    unsigned campaign=duel_scope()?story_duel.character[story_duel.entrant]:
        (projected?saved_play[0x44]:psx_mod_read_byte(PLAY+0x44));
    return stage==18 && psx_mod_read_word(0x800EE970u)==0x20u &&
        ((campaign==1 && kind==0x40u && !subtype) ||
         (!campaign && kind==0x41u && subtype<=1));
}
static void story_roster_predicate(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(!enrolled || failed || busy || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=18 ||
       psx_mod_read_byte(PLAY+13) || psx_mod_read_word(0x800EE970u)!=0x20u)return;
    int opponent=address==0x800F4D90u &&
        psx_mod_read_word(0x800F4D7Cu)==0x3C02800Au &&
        psx_mod_read_word(0x800F4D80u)==0x8043A0A2u &&
        psx_mod_read_word(address)==0x14600005u &&
        psx_mod_read_word(0x800FD36Cu)==0x800F4D7Cu &&
        actor>=0x80092090u && actor<0x80093DD0u && !((actor-0x80092090u)%0x9Cu) &&
        cpu->gpr[16]==actor && psx_mod_read_byte(actor) &&
        psx_mod_read_byte(actor+1)==0x40 && !psx_mod_read_byte(actor+2);
    int producer=address==0x800F8BDCu &&
        psx_mod_read_word(0x800F8BC0u)==0x3C02800Au &&
        psx_mod_read_word(0x800F8BC4u)==0x8043A0A2u &&
        psx_mod_read_word(address)==0x14600031u &&
        actor>=0x80010000u && actor<0x801FFFD0u && !(actor&3u) &&
        cpu->gpr[18]==actor && psx_mod_read_byte(actor);
    int image=address==0x80056848u &&
        psx_mod_read_word(0x80056840u)==0x8043A0A2u &&
        psx_mod_read_word(address)==0x14600004u &&
        psx_mod_read_word(0x80056850u)==0x2402008Du &&
        psx_mod_read_word(0x80056858u)==0xA062D76Au &&
        psx_mod_read_word(0x8005685Cu)==0x2402008Cu &&
        psx_mod_read_word(0x80056864u)==0xA062D76Au;
    if(!opponent && !producer && !image)return;
    /* BIN28's opponents and original56708's door sheet selector ask which
     * campaign is active. P2 projection must not retire the opponent or load
     * the other campaign's DAT140/141 image over its authored native tiles.
     * Replace only the loaded predicate; original allocation, resource load,
     * targeting and scene initialization keep their native behavior. */
    unsigned campaign=duel_scope()?story_duel.character[story_duel.entrant]:
        (projected?saved_play[0x44]:psx_mod_read_byte(PLAY+0x44));
    if(campaign>1)return;
    if(cpu->gpr[3]!=campaign)
        psx_mod_counter_add("mmx5.coop.story-roster-predicate-corrections",1);
    cpu->gpr[3]=campaign;
}
static void ai_redirect(CPUState *cpu,uint32_t address) {
    if(!ai_pool_call || ai_call || projected) {balance_redirect(cpu,address);return;}
    uint32_t base=address==0x80022044u?0x80092090u:0x80096D98u;
    unsigned capacity=address==0x80022044u?48u:32u;
    uint32_t actor=cpu->gpr[4],function=cpu->gpr[5];
    if(actor<base || actor>=base+capacity*0x9Cu || (actor-base)%0x9Cu || ai_pending ||
       (function&3u) || function<0x80010000u || function>=0x80200000u)return;
    if(!split_available() && !duel_scope() && !both_door_pending_boss(actor)) {balance_redirect(cpu,address);return;}
    ai_pending=actor;ai_function=function;cpu->gpr[5]=AI_DISPATCH;
}
static int midboss_admission(CPUState *cpu,uint32_t address) {
    unsigned second_drop=address==0x800EFF84u;
    unsigned water_drain=address==0x800EFE20u;
    if((address!=0x800EFC34u && !second_drop && !water_drain) ||
       area_call || busy || projected || !enrolled || failed ||
       psx_mod_local_view_scope() || scene_owner || car_context() ||
       life.status[0]!=MMX5_COOP_ALIVE || life.status[1]!=MMX5_COOP_ALIVE ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=3 ||
       psx_mod_read_byte(PLAY+13) || psx_mod_read_word(0x800EE970u)!=8 ||
       psx_mod_read_word(address)!=((second_drop || water_drain)?0x3C03800Au:0x27BDFFE8u) ||
       psx_mod_read_word(second_drop?0x80100E28u:water_drain?0x80100E14u:0x80100E00u)!=address ||
       psx_mod_read_word(0x80100E04u)!=0x800EFCACu ||
       psx_mod_read_word(0x800EFC88u)!=0x28420BD0u ||
       (second_drop && psx_mod_read_word(0x800EFF98u)!=0x284212D1u) ||
       (water_drain && psx_mod_read_word(0x800EFE34u)!=0x28420CBDu))return 0;
    uint32_t actor=cpu->gpr[4];
    int threshold=second_drop?4817:water_drain?3261:3024;
    if(actor<0x80010000u || actor>=0x801FFFD0u || (actor&3u) ||
       psx_mod_read_byte(actor+5)!=(second_drop?4u:3u) ||
       psx_mod_read_byte(actor+6)!=(second_drop?1u:water_drain?5u:0u) ||
       (water_drain && !(psx_mod_read_byte(PLAY+0x26)&4u)) ||
       psx_mod_read_byte(PLAYER+5)==0x14 || second.body[4]!=1 ||
       second.body[5]==0x14 ||
       (int16_t)psx_mod_read_half(PLAYER+10)>=threshold ||
       (int16_t)(second.body[10]|second.body[11]<<8)<threshold)return 0;
    /* BIN4's CP3 controller reads only PLAYER.x before original stand14.
     * Run that admission once in the leading P2 context. script_begin then
     * owns the subsequent native stage passes: EFC34 must see that same
     * stand14 again before EFCAC admits the genuine two-bank class1E fight.
     * The later EFF84 gate likewise admits the second vertical drop at4817.
     * After the actual second defeat, EFE20 admits the water drain at3261;
     * this stand must likewise belong to the hero crossing that boundary.
     * Transfer either admitting camera exactly as a P2-owned door does. */
    uint8_t canonical[0xFC];unsigned camera_owner=split_owner;
    int own_camera=split_views && split_valid;
    if(own_camera) {
        capture(CAMERA,canonical,sizeof canonical);
        project(CAMERA,split_cameras[1],sizeof canonical);split_owner=2;
    }
    area_call=1;uint8_t campaign=psx_mod_read_byte(PLAY+0x44);
    enter_second();world_owner=1;
    psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    world_owner=0;leave_second_context(1);area_call=0;
    if(own_camera) {
        capture(CAMERA,split_cameras[1],sizeof canonical);
        if(scene_owner!=2) {project(CAMERA,canonical,sizeof canonical);split_owner=camera_owner;}
    }
    if(scene_owner==2)psx_mod_counter_add(water_drain?"mmx5.coop.p2-water-drain-admissions":
                                                      "mmx5.coop.p2-midboss-admissions",1);
    cpu->gpr[2]=result;return 1;
}
static int whale_drain_physics(CPUState *cpu,uint32_t address) {
    if(area_call || busy || !enrolled || failed || psx_mod_local_view_scope() ||
       cpu->gpr[4]!=PLAYER || cpu->gpr[31]!=0x800EFF14u ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=3 ||
       psx_mod_read_byte(PLAY+13) || psx_mod_read_byte(PLAY+0x1D)!=4 ||
       psx_mod_read_word(0x800EE970u)!=8 ||
       psx_mod_read_word(0x800EFF0Cu)!=0x0C00ED97u ||
       psx_mod_read_word(0x8003B668u)!=0x0C00F552u)return 0;
    /* The completed native drain resets only its resident body's movement
     * constants. Refresh the other living body with the same original
     * initializer, at this exact completion, after checkpoint4 is committed.
     * Each hero retains its armor/Parts modifiers. A departed passenger is
     * restored from scene_body after the drain: refresh that retained body,
     * rather than the hidden beam that the return will overwrite. No water
     * flag, position, health, animation or shared controller advances twice. */
    area_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    unsigned resident=projected?1u:0u,other=resident^1u,owner=world_owner;
    if(both_living(other)) {
        uint8_t play[sizeof saved_play],camera[sizeof saved_camera];
        capture(PLAY,play,sizeof play);capture(CAMERA,camera,sizeof camera);
        both_project(other);world_owner=other;
        uint8_t hidden[sizeof scene_body];
        int retained=scene_owner==resident+1u && (scene_phase==2 || scene_phase==3) &&
            scene_body[4]==1 && (scene_body[0x5C]&0x7Fu);
        if(retained) {
            capture(PLAYER,hidden,sizeof hidden);
            project(PLAYER,scene_body,sizeof scene_body);
        }
        guest(cpu,address,PLAYER,0);
        if(retained) {
            capture(PLAYER,scene_body,sizeof scene_body);
            project(PLAYER,hidden,sizeof hidden);
        }
        both_project(resident);world_owner=owner;
        project(PLAY,play,sizeof play);project(CAMERA,camera,sizeof camera);
        psx_mod_counter_add("mmx5.coop.water-drain-partner-physics",1);
    }
    area_call=0;cpu->gpr[2]=result;return 1;
}
static int whale_authored_camera(void) {
    /* BIN4's whale controller owns the entire autoscrolling room, including
     * its current bounds and the whale/background atlas. A second ordinary
     * follow camera retains obsolete bounds and separates the whale parts.
     * Phase4's ordinary hallway and final boss CP6 retain independent
     * follow/area commands; they are not forced whale scrolling. */
    if(psx_mod_read_byte(PLAY+12)!=3 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=8 ||
       psx_mod_read_word(0x800EF584u)!=0x3C05800Au ||
       psx_mod_read_word(0x800F0260u)!=0x3C02800Au ||
       psx_mod_read_word(0x800F02CCu)!=0x3C02800Au)return 0;
    for(uint32_t actor=0x800C94A8u;actor<0x800C9568u;actor+=0x30u) {
        unsigned phase=psx_mod_read_byte(actor+5);
        if(psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==1 &&
           psx_mod_read_byte(actor+4)==1 && psx_mod_read_word(actor+0x10)==0x80100D88u &&
           phase>0 && phase<6 && phase!=4)return 1;
    }
    return 0;
}
static int whale_independent_exit(void) {
    if(!whale_authored_camera() || !(psx_mod_read_byte(PLAY+0x26)&4u))return 0;
    for(uint32_t actor=0x800C94A8u;actor<0x800C9568u;actor+=0x30u)
        if(psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==1 &&
           psx_mod_read_byte(actor+4)==1 && psx_mod_read_byte(actor+5)==3 &&
           psx_mod_read_word(actor+0x10)==0x80100D88u)return 1;
    return 0;
}
static void whale_final_follow(CPUState *cpu,uint32_t address) {
    (void)address;uint32_t actor=cpu->gpr[16];
    if(!split_available() || cpu->gpr[2]!=CAMERA || cpu->gpr[17]!=1 ||
       !whale_authored_camera() ||
       actor<0x80092090u || actor>=0x80093DD0u || (actor-0x80092090u)%0x9Cu ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x1E ||
       psx_mod_read_byte(actor+2)!=16 || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+5)!=5 || (psx_mod_read_byte(actor+0x5C)&127u) ||
       (psx_mod_read_byte(PLAY+0x2A)&5u)!=5u ||
       psx_mod_read_word(0x80102E9Cu)!=0x800FF494u ||
       psx_mod_read_word(0x80102E4Cu)!=0x800FD54Cu)return;
    /* FD63C resumes the primary callback after the actual third final hit,
     * but CP5 still carries Y960 bounds from before its authored ascent.
     * Keep the reached room locked through the short climb to the first
     * boss door. Allowing ordinary follow here instead follows a falling
     * hull down into the old checkpoint. The original door/area commands
     * replace these bounds on entry; floor, gravity and pits stay native. */
    uint16_t y=psx_mod_read_half(CAMERA+14);
    psx_mod_write_half(CAMERA+0x20,y);psx_mod_write_half(CAMERA+0x28,y);
    psx_mod_write_half(CAMERA+0x22,y);psx_mod_write_half(CAMERA+0x2A,y);
    psx_mod_counter_add("mmx5.coop.whale-final-follow-bounds",1);
}
static int pegasus_platform_camera(void) {
    /* These original parents own one moving support and its camera through
     * authored travel. F36E8, F3930 and F3B04/F3C34 advance the
     * native bounds; each program reaches idle phase8 itself. Up-lift stops
     * 2/4 wait for bombs on a side ledge: their final native Y bounds remain
     * fixed while ordinary private X follow keeps that explorer in view.
     * Admission already requires both living heroes' actual top contact. */
    if(psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=8 ||
       psx_mod_read_byte(PLAY+13) || psx_mod_read_word(0x800EE970u)!=0x0Fu ||
       psx_mod_read_word(0x800F9C78u)!=0x800F4A20u ||
       psx_mod_read_word(0x800F9C7Cu)!=0x800F3930u ||
       psx_mod_read_word(0x800F9C80u)!=0x800F4A5Cu)return 0;
    for(uint32_t actor=0x800C6A10u;actor<0x800C9310u;actor+=0xA4u) {
        unsigned subtype=psx_mod_read_byte(actor+2);
        if(psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==0x12 &&
           subtype<=2 && psx_mod_read_byte(actor+4)==1 &&
           psx_mod_read_byte(actor+5)==subtype+5) {
            unsigned stop=psx_mod_read_byte(actor+6);
            if(subtype==2 && (stop==2 || stop==4))continue;
            return 1;
        }
    }
    return 0;
}
static void pegasus_camera_command(const uint8_t canonical[0xFC]) {
    /* The support owns the progressive room bounds, while each rider owns
     * the follow origin within them. Copying the resident player's Y/history
     * locks P2 vertically to P1 during travel, including their jumps. Keep
     * both private origins and let the original follow/layer callbacks use
     * the same native bounds. No support/world step repeats. */
    if(!split_views || !split_valid)return;
    for(unsigned seat=0;seat<2;++seat)if(life.status[seat]==MMX5_COOP_ALIVE) {
        uint8_t *view=split_cameras[seat];
        memcpy(view,canonical,8);
        memcpy(view+0x1C,canonical+0x1C,4);
        memcpy(view+0x24,canonical+0x24,8);
        view[0x47]=canonical[0x47];
    }
}
static int whale_player_bounds(CPUState *cpu,uint32_t address) {
    if(area_call || busy || projected || !enrolled || failed ||
       psx_mod_local_view_scope() || !whale_authored_camera())return 0;
    /* These three original leaves only constrain PLAYER against the one
     * authored camera (including native crush damage). Apply the same rule
     * to the other living foot actor, without replaying the whale controller,
     * stage timer, scrolling or enemy update. */
    area_call=1;uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    if(life.status[1]==MMX5_COOP_ALIVE && second.body[4]==1 && !scene_passenger(1)) {
        enter_second();guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);leave_second();
        psx_mod_counter_add("mmx5.coop.p2-whale-player-bounds",1);
    }
    area_call=0;cpu->gpr[2]=result;return 1;
}
static void chain_platform_proximity(CPUState *cpu,uint32_t address) {
    (void)address;uint32_t actor=cpu->gpr[16];
    if(projected || busy || !enrolled || failed || scene_owner ||
       psx_mod_local_view_scope() || life.status[1]!=MMX5_COOP_ALIVE ||
       second.body[4]!=1 || second.body[0xF8] || psx_mod_read_byte(PLAYER+0xF8) ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=6 ||
       psx_mod_read_byte(PLAY+13) || psx_mod_read_word(0x800EE970u)!=12 ||
       psx_mod_read_word(0x800F1B78u)!=0x27BDFFE0u ||
       psx_mod_read_word(0x800F1C54u)!=0x0C00C903u ||
       psx_mod_read_word(0x800F3E68u)!=0x800F1934u || !solid_record(actor) ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x10 ||
       psx_mod_read_byte(actor+4)!=1)return;
    int x=(int16_t)psx_mod_read_half(actor+10),first_x=(int16_t)cpu->gpr[4];
    if((int16_t)cpu->gpr[3]!=x || first_x!=(int16_t)psx_mod_read_half(PLAYER+10) ||
       (first_x-x>=-48 && first_x-x<=48))return;
    int second_x=(int16_t)(le32(second.body+8)>>16);
    if(second_x-x < -48 || second_x-x > 48)return;
    /* F1B78 advances the shared platform once, then admits solid contact
     * only within48 pixels of PLAYER. Include the other living hero in that
     * predicate. Original3240C still performs both real contacts/carry through
     * the existing per-player histories, in its original update order. */
    cpu->gpr[4]=(uint32_t)(int32_t)second_x;
    psx_mod_counter_add("mmx5.coop.p2-chain-platform-proximity",1);
}
static int grizzly_chain_actor(uint32_t actor) {
    return grizzly_stage() && actor>=0x8009E7A0u && actor<0x8009F9A0u &&
        !((actor-0x8009E7A0u)%0x30u) &&
        psx_mod_read_word(0x800F7B14u)==0x27BDFFE8u &&
        psx_mod_read_word(0x800FD42Cu)==0x800F7BECu &&
        psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==9 &&
        psx_mod_read_byte(actor+2)<4 && psx_mod_read_byte(actor+4)==1;
}
unsigned mmx5_coop_grizzly_chain_retained(uint32_t actor) {
    if(!split_views || !split_valid || !enrolled || failed || scene_owner ||
       !both_living(0) || !both_living(1) || !grizzly_chain_actor(actor))return 0;
    Mmx5CoopViewActor actors[2];split_actors(actors);
    unsigned resident=mmx5_coop_view_nearest(actors,
        (int16_t)psx_mod_read_half(actor+10),(int16_t)psx_mod_read_half(actor+14),0);
    return actors[resident^1u].x-actors[resident].x>640 &&
        actors[resident^1u].x-(int16_t)psx_mod_read_half(actor+10)>320?resident+1u:0;
}
static int grizzly_chain_tick(CPUState *cpu,uint32_t address) {
    (void)address;uint32_t actor=cpu->gpr[4];
    if(area_call || !split_available() || !grizzly_chain_actor(actor))return 0;
    unsigned retained=mmx5_coop_grizzly_chain_retained(actor);
    if(!retained) {
        uint32_t parent=psx_mod_read_word(actor+0x20);
        if(parent>=0x80092090u && parent<0x80093DD0u &&
           !((parent-0x80092090u)%0x9Cu) && psx_mod_read_byte(parent) &&
           psx_mod_read_byte(parent+1)==0x10 && psx_mod_read_byte(parent+2)<6)return 0;
        /* Its native parent slot can already have been freed/reused while
         * the old resident waited. Retire the retained child when they leave. */
        area_call=1;uint32_t result=guest(cpu,0x8002D6FCu,actor,0);area_call=0;
        cpu->gpr[2]=result;return 1;
    }
    /* The cannon's native train parent retires after its countdown, leaving
     * its last electric tiles in the distant old pane. Keep already-admitted
     * chain programs until the resident leaves, without dereferencing the
     * retired/reusable parent. One original clock still owns map/contact art. */
    uint32_t bounds=0x800FD408u+psx_mod_read_byte(actor+2)*6u;
    area_call=1;cpu->gpr[6]=psx_mod_read_half(bounds+4);
    uint32_t outside=guest(cpu,0x8002D938u,actor,psx_mod_read_half(bounds+2));
    if(!outside && !actor_byte(retained-1u,0xF7)) {
        guest(cpu,0x800F79C0u,actor,0);
        psx_mod_counter_add("mmx5.coop.trailing-train-chain-ticks",1);
    }
    area_call=0;cpu->gpr[2]=0;return 1;
}
static void area_redirect(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=cpu->gpr[4],function=cpu->gpr[3];
    if(!area_call && !area_actor && !busy && !projected && !scene_owner && split_available() &&
       grizzly_stage() && life.status[1]==MMX5_COOP_ALIVE && second.body[4]==1 &&
       actor>=0x800C94A8u && actor<0x800C9568u && !((actor-0x800C94A8u)%0x30u) &&
       function==0x800EF3ACu && !psx_mod_read_byte(actor+1) &&
       psx_mod_read_byte(actor+4)==1 && psx_mod_read_word(actor+0x10)==0x800FBFA4u) {
        unsigned phase=psx_mod_read_byte(actor+5);
        int timer=!phase && (int16_t)psx_mod_read_half(PLAYER+10)<0x1CF0 &&
            (int16_t)(second.body[10]|second.body[11]<<8)>=0x1CF0;
        int escape=phase==4 && (int16_t)psx_mod_read_half(CAMERA+0x5E)<0xFD0 &&
            (int16_t)(split_cameras[1][0x5E]|split_cameras[1][0x5F]<<8)>=0xFD0;
        if(timer || escape) {
            area_actor=actor;area_function=function;cpu->gpr[3]=AREA_DISPATCH;
            psx_mod_counter_add(timer?"mmx5.coop.p2-train-countdown-admissions":
                "mmx5.coop.p2-escape-train-admissions",1);return;
        }
    }
    if(area_call || area_actor || busy || projected || !enrolled || failed ||
       psx_mod_local_view_scope() || life.status[0]!=MMX5_COOP_ALIVE ||
       life.status[1]!=MMX5_COOP_ALIVE || scene_owner ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=2 ||
       psx_mod_read_byte(PLAY+13) || psx_mod_read_word(0x800EE970u)!=6 ||
       psx_mod_read_word(0x800EE9B0u)!=0x28A51521u)return;
    if(actor<0x800C94A8u || actor>=0x800C9568u || (actor-0x800C94A8u)%0x30u ||
       function!=0x800EE988u)return;
    /* BIN2's reserved stage controller requests its actual exit only after
     * PLAYER.x>5408. Select P2 at that boundary; the original controller,
     * stage state machine and loader still produce every transition flag. */
    int first_x=(int16_t)psx_mod_read_half(PLAYER+10);
    int second_x=(int16_t)(second.body[10]|second.body[11]<<8);
    if(first_x>5408 || second_x<=5408)return;
    area_actor=actor;area_function=function;cpu->gpr[3]=AREA_DISPATCH;
}
static uint32_t rescue_contact(uint32_t actor,uint32_t function) {
    /* The original class4 Reploid program is relocated with each stage.
     * Match its native dispatcher, DAT85 initialization and contact leaf,
     * rather than intercepting every geometry query or guessing stage IDs. */
    if(actor<0x800C6A10u || actor>=0x800C9310u || (actor-0x800C6A10u)%0xA4u ||
       psx_mod_read_byte(actor+1)!=4 || function<0x800EE970u || function>0x801FFE00u ||
       psx_mod_read_word(function)!=0x27BDFFE8u ||
       (psx_mod_read_word(function+0x18)&0xFFFF0000u)!=0x3C030000u ||
       (psx_mod_read_word(function+0x24)&0xFFFF0000u)!=0x24630000u ||
       psx_mod_read_word(function+0x3C)!=0x00A0F809u ||
       psx_mod_read_word(function+0x54)!=0x0C00BED6u)return 0;
    uint32_t table=(psx_mod_read_word(function+0x18)<<16)+(int16_t)psx_mod_read_half(function+0x24);
    if(table<0x800EE970u || table>0x801FFFF0u)return 0;
    uint32_t init=psx_mod_read_word(table),update=psx_mod_read_word(table+4);
    if(init<0x800EE970u || init>0x801FFF00u || update<0x800EE970u || update>0x801FFF00u ||
       psx_mod_read_word(init+0x88)!=0x24040085u)return 0;
    uint32_t call=psx_mod_read_word(update+0x4C);
    if((call&0xFC000000u)!=0x0C000000u)return 0;
    uint32_t contact=0x80000000u|((call&0x03FFFFFFu)<<2);
    if(contact<0x800EE970u || contact>0x801FFF00u ||
       psx_mod_read_word(contact+4)!=0x3C03800Au ||
       psx_mod_read_word(contact+8)!=0x2465A0A0u ||
       psx_mod_read_word(contact+0x28)!=0x0C00BA01u ||
       psx_mod_read_word(contact+0x4C)!=0xA0620045u)return 0;
    return contact;
}
static uint32_t rescue_tick(CPUState *cpu,uint32_t actor,uint32_t function,uint32_t contact) {
    uint8_t locks[8],effect_lock=psx_mod_read_byte(PLAY+0x19);
    capture(PLAY+0x10,locks,sizeof locks);
    unsigned slot=0;while(slot<128 && pickup_owners[slot].actor!=actor)++slot;
    unsigned owner=slot<128?pickup_owners[slot].seat:0;
    unsigned before=psx_mod_read_byte(actor+4);
    /* Advance the NPC once. After an actual rescue its original healing
     * program must keep the collector even if the other hero moves closer. */
    uint32_t result=pickup_as_seat(cpu,function,actor,owner);
    if(before==1 && psx_mod_read_byte(actor+4)==1 &&
       life.status[1]==MMX5_COOP_ALIVE && !scene_passenger(1)) {
        /* P1 already had its native admission attempt. The extra P2 call is
         * only the original contact/reward leaf, never a second NPC tick. */
        result=pickup_as_seat(cpu,contact,actor,1);owner=1;
    }
    if(before==1 && psx_mod_read_byte(actor+4)==2) {
        if(slot==128) {slot=0;while(slot<128 && pickup_owners[slot].actor)++slot;}
        if(slot==128)failed=1;
        else {pickup_owners[slot].actor=actor;pickup_owners[slot].seat=(uint8_t)owner;}
        psx_mod_counter_add(owner?"mmx5.coop.p2-reploid-rescues":"mmx5.coop.p1-reploid-rescues",1);
    }
    if(slot<128 && !psx_mod_read_byte(actor))pickup_owners[slot].actor=0;
    /* The native Reploid's healing program uses545DC/54610, like health
     * items. Its global pause makes split_camera_finish share P1's camera,
     * clamping a distant collector into terrain when healing ends. Keep the
     * actual contact, extra life and recipient's healing ticks, with the
     * surrounding world locks preserved throughout admission/completion. */
    project(PLAY+0x10,locks,sizeof locks);psx_mod_write_byte(PLAY+0x19,effect_lock);
    return result;
}
static void area_update(CPUState *cpu,uint32_t address) {
    (void)address;uint32_t actor=area_actor,function=area_function;
    area_actor=area_function=0;if(!actor || !function || area_call)return;
    uint32_t rescue=rescue_contact(actor,function);
    if(rescue) {
        area_call=1;uint32_t result=rescue_tick(cpu,actor,function,rescue);area_call=0;
        cpu->gpr[2]=result;return;
    }
    uint8_t canonical[0xFC];unsigned camera_owner=split_owner;
    int train=function==0x800EF3ACu && grizzly_stage() && split_available();
    if(train) {
        /* EEDEC's countdown admission reads PLAYER.x; EF920's escape train
         * reads layer1.x. The one native controller needs the same admitting
         * hero and camera, including its shared stand/control sequence. */
        capture(CAMERA,canonical,sizeof canonical);project(CAMERA,split_cameras[1],sizeof canonical);split_owner=2;
    }
    area_call=1;uint8_t campaign=psx_mod_read_byte(PLAY+0x44);
    enter_second();world_owner=1;
    psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
    uint32_t result=guest(cpu,function,actor,cpu->gpr[5]);
    world_owner=0;leave_second_context(1);area_call=0;
    if(train) {
        capture(CAMERA,split_cameras[1],sizeof canonical);
        if(scene_owner!=2) {project(CAMERA,canonical,sizeof canonical);split_owner=camera_owner;}
    }
    psx_mod_counter_add("mmx5.coop.p2-area-controller",1);cpu->gpr[2]=result;
}
/* Original BIN18 EF57C reads only current XY and the two collision boxes.
 * Keep its integer/wrap rules, including the inset16 and unused X offsets,
 * without projecting another body or replaying any part of the world. */
static int refight_pad_contact(uint32_t actor,unsigned seat) {
    uint32_t body_box=both_body_word(seat,0x68),pad_box=psx_mod_read_word(actor+0x68);
    if(!body_box || !pad_box)return 0;
    int x=(int16_t)(both_body_word(seat,8)>>16);
    int pad_x=(int16_t)psx_mod_read_half(actor+10);
    int half_width=psx_mod_read_byte(body_box+2),pad_width=psx_mod_read_byte(pad_box+2);
    if((int16_t)(x-half_width)<(int16_t)(pad_x-(pad_width-16)) ||
       (int16_t)(pad_x+(pad_width-16))<(int16_t)(x+half_width))return 0;
    int y=(int16_t)((int16_t)(both_body_word(seat,12)>>16)+(int8_t)psx_mod_read_byte(body_box+1));
    int pad_y=(int16_t)((int16_t)psx_mod_read_half(actor+14)+(int8_t)psx_mod_read_byte(pad_box+1));
    int separation=pad_y-y;if(separation<0)separation=-separation;
    /* NativeEF630 folds this distance through signed16 before comparing. */
    separation=(int16_t)separation;
    return separation<=(int)(psx_mod_read_byte(body_box+3)+psx_mod_read_byte(pad_box+3));
}
static int refight_pad_admission(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(!both_door_players || busy || !enrolled || failed || psx_mod_local_view_scope() ||
       (projected && !world_owner) || scene_owner ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=12 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=22 ||
       psx_mod_read_word(0x800EF7A4u)!=0x27BDFFE0u ||
       psx_mod_read_word(0x800F75FCu)!=address ||
       psx_mod_read_word(0x800F70C8u)!=0x800EF65Cu ||
       actor<0x800C6A10u || actor>=0x800C9310u || (actor-0x800C6A10u)%0xA4u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x17 ||
       psx_mod_read_byte(actor+2)<2 || psx_mod_read_byte(actor+2)>10 ||
       psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+5) || psx_mod_read_byte(actor+6) ||
       !both_living(0) || !both_living(1))return 0;
    unsigned first_hit=refight_pad_contact(actor,0),second_hit=refight_pad_contact(actor,1);
    if(first_hit==second_hit)return 0; /* Preserve native no-contact and both-contact paths. */
    /* Skip only idle admission, before native XY snap/stand14. EF750 still
     * animates/draws the pad and EF6E4 still resolves its shared solid once.
     * No first-touch latch: a fallen/withdrawn partner releases the wait. */
    psx_mod_counter_add("mmx5.coop.both-refight-pad-waits",1);
    cpu->gpr[2]=0;return 1;
}
static void refight_pad_redirect(CPUState *cpu,uint32_t address) {
    (void)address;uint32_t actor=cpu->gpr[4],function=cpu->gpr[5];
    if(!area_call && !area_actor && !busy && enrolled && !failed &&
       !psx_mod_local_view_scope() && psx_mod_read_byte(PLAY)==10 && rescue_contact(actor,function)) {
        area_actor=actor;area_function=function;cpu->gpr[5]=AREA_DISPATCH;return;
    }
    if(function!=0x800EF65Cu || area_call || area_actor || busy || projected || !enrolled || failed ||
       psx_mod_local_view_scope() || scene_owner || life.status[1]!=MMX5_COOP_ALIVE || second.body[4]!=1 ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=12 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_byte(PLAY+0x10) || psx_mod_read_byte(PLAY+0x1C) || psx_mod_read_byte(PLAY+0x0F) ||
       psx_mod_read_word(0x800EE970u)!=22 || psx_mod_read_word(0x800EF65Cu)!=0x27BDFFE8u ||
       psx_mod_read_word(0x800EF57Cu)!=0x8CA60068u)return;
    if(actor<0x800C6A10u || actor>=0x800C9310u || (actor-0x800C6A10u)%0xA4u ||
       psx_mod_read_byte(actor+1)!=23 || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+5) || psx_mod_read_byte(actor+6) ||
       psx_mod_read_byte(actor+2)<2 || psx_mod_read_byte(actor+2)>10)return;
    /* BIN18's idle pad tests only the singleton PLAYER at EF7C0. Preserve
     * native P1 priority, then test that same original collision routine for
     * P2. The existing shared-world scope runs the native pad once under its
     * owner; 3D29C acquires the script, and native code selects the checkpoint
     * and performs the fade/loader. No transition flag is manufactured here. */
    if(life.status[0]==MMX5_COOP_ALIVE && psx_mod_read_word(PLAYER+0x68) &&
       guest(cpu,0x800EF57Cu,actor,PLAYER))return;
    if(!le32(second.body+0x68))return;
    enter_second();uint32_t hit=guest(cpu,0x800EF57Cu,actor,PLAYER);leave_second();
    if(!hit)return;
    area_actor=actor;area_function=function;cpu->gpr[5]=AREA_DISPATCH;
    psx_mod_counter_add("mmx5.coop.p2-refight-pad-handoffs",1);
}
static int falling_slab_trigger(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(area_call || projected || busy || !enrolled || failed || psx_mod_local_view_scope() ||
       scene_owner || life.status[1]!=MMX5_COOP_ALIVE || second.body[4]!=1 ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12) || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800F588Cu)!=0x27BDFFE8u ||
       psx_mod_read_word(0x800FB29Cu)!=0x800F588Cu ||
       actor<0x800C6A10u || actor>=0x800C9310u || (actor-0x800C6A10u)%0xA4u ||
       psx_mod_read_byte(actor+1)!=1 || psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+6))return 0;
    unsigned subtype=psx_mod_read_byte(actor+2);
    if(subtype>2)return 0;
    int threshold=subtype==1?2880:subtype==2?3048:(int16_t)psx_mod_read_half(actor+10);
    if((life.status[0]==MMX5_COOP_ALIVE && (int16_t)psx_mod_read_half(PLAYER+10)>=threshold) ||
       (int16_t)(second.body[10]|second.body[11]<<8)<threshold)return 0;
    /* BIN0 F588C tests only PLAYER.x before arming class1's authored fall.
     * Project just this native substate, once: the parent F5C30 must retain
     * canonical shot queries, immunity gate and dual-player solid contacts.
     * Native code still owns acceleration, phase, quake and retirement. */
    uint8_t campaign=psx_mod_read_byte(PLAY+0x44);
    area_call=1;enter_second();world_owner=1;
    /* The falling slab is shared scenery. Its queued dust/quake commands
     * keep the stage palette and campaign while testing P2's position. */
    psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    world_owner=0;leave_second_context(1);area_call=0;
    psx_mod_counter_add("mmx5.coop.p2-falling-slab-triggers",1);
    cpu->gpr[2]=result;return 1;
}
static int intro_boss_text_skip(CPUState *cpu,uint32_t address) {
    if(address!=0x8002385Cu || !enrolled || failed || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12) || psx_mod_read_byte(PLAY+13) ||
       cpu->gpr[31]!=0x800F825Cu || cpu->gpr[4] || cpu->gpr[5]!=255u ||
       psx_mod_read_word(0x800EE970u)!=4u ||
       psx_mod_read_word(0x800FB4E4u)!=0x800F8224u ||
       psx_mod_read_word(0x800F8244u)!=0x10600011u ||
       psx_mod_read_word(0x800F8254u)!=0x0C008E17u ||
       psx_mod_read_word(0x800F82A0u)!=0x3C028009u ||
       psx_mod_read_byte(0x80091C44u))return 0;
    /* Skip only Sigma's original post-reveal text request. The statue,
     * explosions and white flash have already completed; the original next
     * callback sees idle text, clears its own holds and starts the fight. */
    psx_mod_counter_add("mmx5.coop.intro-boss-dialogue-skips",1);
    cpu->gpr[2]=0;return 1;
}
static int intro_reunion_skip(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(address!=0x800F8170u || area_call || !enrolled || failed ||
       psx_mod_local_view_scope() || psx_mod_read_byte(PLAY)!=10 ||
       psx_mod_read_byte(PLAY+12) || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_byte(PLAY+0x2E) ||
       psx_mod_read_word(0x800EE970u)!=4 ||
       psx_mod_read_word(0x800F7EF0u)!=0x27BDFFE0u ||
       psx_mod_read_word(0x800FB4A4u)!=0x800F7FC4u ||
       psx_mod_read_word(0x800FB4A8u)!=0x800F6F70u ||
       psx_mod_read_word(0x800FB4B8u)!=0x800F80B8u ||
       psx_mod_read_word(0x800FB4E0u)!=address ||
       psx_mod_read_word(0x800FB4E4u)!=0x800F8224u ||
       psx_mod_read_word(0x800FB4ECu)!=0x800F82E8u ||
       psx_mod_read_word(address)!=0x27BDFFE8u ||
       psx_mod_read_word(0x800F8198u)!=0x80431C2Eu ||
       psx_mod_read_word(0x800F81A0u)!=0x10600006u ||
       psx_mod_read_word(0x800F81B0u)!=0x24420003u ||
       actor<0x80092090u || actor>=0x80093DD0u || (actor-0x80092090u)%0x9Cu ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x31 ||
       psx_mod_read_byte(actor+2) || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+5)!=2 || psx_mod_read_byte(actor+6)!=1 ||
       psx_mod_read_word(PLAY+0x20)!=actor)return 0;
    uint32_t warning=psx_mod_read_word(actor+0x80);
    if(warning<0x8009E7A0u || warning>=0x8009F9A0u ||
       (warning-0x8009E7A0u)%0x30u || psx_mod_read_byte(warning))return 0;
    /* This original callback already has a replay path: after WARNING
     * retires, nonzeroPLAY2E skips only the staged counterpart/reunion
     * substates1->4. Scope that existing policy to this one co-op call.
     * Original Sigma emergence, text1, HP fill and control release follow;
     * actual defeat and solo dialogue are untouched. */
    uint8_t replay=psx_mod_read_byte(PLAY+0x2E);
    area_call=1;psx_mod_write_byte(PLAY+0x2E,1);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    psx_mod_write_byte(PLAY+0x2E,replay);area_call=0;
    if(psx_mod_read_byte(actor+6)==4)
        psx_mod_counter_add("mmx5.coop.intro-reunion-skips",1);
    cpu->gpr[2]=result;return 1;
}
static int intro_structure_trigger(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(address!=0x800F65C8u || area_call || projected || busy || !enrolled || failed ||
       psx_mod_local_view_scope() || scene_owner ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12) || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=4 ||
       psx_mod_read_word(0x800F6BD4u)!=0x27BDFFE8u ||
       psx_mod_read_word(0x800FB3C4u)!=0x800F6324u ||
       psx_mod_read_word(0x800FB3C8u)!=0x800F6C2Cu ||
       psx_mod_read_word(0x800FB3D0u)!=address ||
       psx_mod_read_word(address)!=0x27BDFFE0u ||
       psx_mod_read_word(0x800F6620u)!=0x8462A0AEu ||
       psx_mod_read_word(0x800F66A0u)!=0x8462A0AEu ||
       psx_mod_read_word(0x800F6700u)!=0x8462A0AEu ||
       actor<0x800C6A10u || actor>=0x800C9310u || (actor-0x800C6A10u)%0xA4u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=3 ||
       psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+5)!=2 ||
       psx_mod_read_byte(actor+6))return 0;
    unsigned subtype=psx_mod_read_byte(actor+2);
    if(subtype>2) {
        /* F64D8 creates only these two collisionless linked pieces. Their
         * native752 admission starts the same motion as parent2, then
         * F677C emits debris and completes them when that parent finishes. */
        if(subtype!=0x7Eu || psx_mod_read_word(actor+0x68))return 0;
        uint32_t parent=psx_mod_read_word(actor+0x7C);
        uint32_t hitbox=psx_mod_read_word(actor+0x54);
        if(parent<0x800C6A10u || parent>=0x800C9310u ||
           (parent-0x800C6A10u)%0xA4u || parent==actor ||
           !psx_mod_read_byte(parent) || psx_mod_read_byte(parent+1)!=3 ||
           psx_mod_read_byte(parent+2)!=2 || psx_mod_read_byte(parent+4)<1 ||
           psx_mod_read_byte(parent+4)>2 || psx_mod_read_byte(parent+5)!=2 ||
           psx_mod_read_word(parent+0x68)!=0x800FB384u ||
           (hitbox!=0x800FB390u && hitbox!=0x800FB394u))return 0;
    }
    int threshold=subtype==0?938:subtype==1?608:752;
    int living0=both_living(0),living1=both_living(1);
    if((!living0 && !living1) ||
       (living0 && (int16_t)psx_mod_read_half(PLAYER+14)>=threshold) ||
       (living1 && (int16_t)(second.body[14]|second.body[15]<<8)>=threshold)) {
        /* Keep the original idle predicate closed until the last living
         * hero has climbed past it. No first-touch latch: death/withdrawal
         * immediately releases the wait for the surviving hero. */
        psx_mod_counter_add("mmx5.coop.intro-structure-waits",1);
        cpu->gpr[2]=0;return 1;
    }
    if(living0)return 0;
    /* BIN0's three original structure predicates read only PLAYER.y.
     * Once all living heroes qualify, retain the canonical native call
     * or use the survivor P2 through this one idle callback. Its parent updates
     * motion, map changes, shared quake and both solid contacts once.
     * Native-linked7E pieces share this predicate only with their exact
     * original parent/collider provenance;70 scenery remains untouched. */
    uint8_t campaign=psx_mod_read_byte(PLAY+0x44);
    area_call=1;enter_second();world_owner=1;
    psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    world_owner=0;leave_second_context(1);area_call=0;
    psx_mod_counter_add("mmx5.coop.p2-intro-structure-triggers",1);
    cpu->gpr[2]=result;return 1;
}
static int corridor_arrow_near(int x,int y) {
    /* BIN3's original F26AC scans this same native coordinate list, skipping
     * index12. These arrows transfer between corridors, not gravity modes. */
    for(unsigned i=0;i<14;++i)if(i!=12) {
        int cx=(int16_t)psx_mod_read_half(0x800F9D0Cu+i*4);
        int cy=(int16_t)psx_mod_read_half(0x800F9D0Eu+i*4);
        if(x>cx-24 && x<cx+24 && y>cy-80 && y<cy+80)return 1;
    }
    return 0;
}
static int corridor_arrow_admission(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(address!=0x800F26ACu || area_call || !enrolled || failed ||
       psx_mod_local_view_scope() || psx_mod_read_byte(PLAY)!=10 ||
       psx_mod_read_byte(PLAY+12)!=2 || psx_mod_read_byte(PLAY+13)!=1 ||
       psx_mod_read_word(0x800EE970u)!=7 || psx_mod_read_word(address)!=0x27BDFFD0u ||
       psx_mod_read_word(0x800F9358u)!=0x800F2B98u ||
       psx_mod_read_word(0x800F9DE0u)!=address ||
       actor<0x800C6A10u || actor>=0x800C9310u || (actor-0x800C6A10u)%0xA4u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x15 ||
       psx_mod_read_byte(actor+2) || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+5)!=1)return 0;
    /* The original controller rearms when its owner leaves the receiver,
     * before the companion's beam has necessarily finished. A second raw
     * admission during that beam bypasses the new source-pair enrollment
     * and returns the companion to the previous corridor's floor. Let the
     * existing return settle before another idle arrow can acquire a hero. */
    if(scene_owner) {
        psx_mod_counter_add("mmx5.coop.corridor-arrow-return-waits",1);
        cpu->gpr[2]=0;return 1;
    }
    if(busy || projected || both_door_active() || car_context() ||
       !both_living(0) || !both_living(1) ||
       psx_mod_read_byte(PLAY+0x10) || psx_mod_read_byte(PLAY+0x1C))return 0;
    unsigned seat=0;
    if(!corridor_arrow_near((int16_t)psx_mod_read_half(PLAYER+10),
                            (int16_t)psx_mod_read_half(PLAYER+14))) {
        if(!corridor_arrow_near((int16_t)(second.body[10]|second.body[11]<<8),
                                (int16_t)(second.body[14]|second.body[15]<<8)))return 0;
        seat=1;
    }
    uint8_t camera[0xFC];unsigned camera_owner=split_owner;
    int own_camera=seat && split_views && split_valid;
    if(own_camera) {
        capture(CAMERA,camera,sizeof camera);
        project(CAMERA,split_cameras[seat],sizeof camera);split_owner=seat+1;
    }
    area_call=1;
    if(seat) {
        uint8_t campaign=psx_mod_read_byte(PLAY+0x44);enter_second();world_owner=1;
        psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
    }
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    /* Native F26AC commits its own lock, matched index, effects and phase.
     * It bypasses 3D29C, so explicitly enroll that observed lock in the
     * existing passenger/owner machinery. Subsequent original stage passes
     * then transfer the admitting hero, and the passenger returns on its
     * native foot-aligned arrival after the original fade/control release. */
    if(psx_mod_read_byte(PLAY+0x10)==2 && psx_mod_read_byte(0x800FA238u)==1 &&
       psx_mod_read_byte(actor+5)==2) {
        scene_begin(seat);
        unsigned index=psx_mod_read_byte(actor+0x7C);
        if(scene_owner==seat+1 && index<14 && index!=12)scene_corridor_arrow=index+1;
        psx_mod_counter_add(seat?"mmx5.coop.p2-corridor-arrow-admissions":
                                "mmx5.coop.p1-corridor-arrow-admissions",1);
    }
    if(seat) {world_owner=0;leave_second_context(1);}
    area_call=0;
    if(own_camera) {
        capture(CAMERA,split_cameras[seat],sizeof camera);
        if(scene_owner!=seat+1) {project(CAMERA,camera,sizeof camera);split_owner=camera_owner;}
    }
    cpu->gpr[2]=result;return 1;
}
static int necrobat_platform_riders(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];SolidHistory *h=solid_record(actor);
    if(address!=0x800EFA30u || area_call || busy || !enrolled || failed ||
       (projected && !world_owner) || psx_mod_local_view_scope() || scene_owner ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=2 ||
       psx_mod_read_byte(PLAY+13) || psx_mod_read_word(0x800EE970u)!=6 ||
       !h || !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1) ||
       psx_mod_read_byte(actor+2)!=5 || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+6)!=5 || psx_mod_read_byte(actor+0x85) ||
       psx_mod_read_word(actor+0x80) ||
       psx_mod_read_word(0x800F44D4u)!=address ||
       psx_mod_read_word(0x800EFB20u)!=0x92020072u ||
       psx_mod_read_word(0x800EFB38u)!=0x8444A0AAu)return 0;
    unsigned sides=psx_mod_read_byte(actor+0x72),living=0;
    int boarded=1;
    for(unsigned seat=0;seat<2;++seat)if(both_living(seat)) {
        ++living;
        if(!((seat?h->sides:sides)&8u))boarded=0;
    }
    if(!living || !boarded) {
        /* Only the idle route leaf waits. Its parent still animates, draws,
         * resolves both solid contacts and advances the world once. No
         * first-rider latch: a dead/absent partner releases the survivor.
         * Fresh entry first raises the lift from below the ledge. When that
         * native approach timer expires, match EFB4C's stopped velocities;
         * skipping the leaf alone leaves its upward motion running. */
        psx_mod_write_word(actor+0x20,0);psx_mod_write_word(actor+0x24,0);
        psx_mod_counter_add("mmx5.coop.necrobat-platform-rider-waits",1);
        cpu->gpr[2]=0;return 1;
    }
    /* Native EFB20 reads the first body's retained top contact. A sole P2
     * rider has that contact in the existing second-body solid history. */
    psx_mod_write_byte(actor+0x72,(uint8_t)(sides|8u));
    area_call=1;uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);area_call=0;
    psx_mod_write_byte(actor+0x72,(uint8_t)sides);
    psx_mod_counter_add("mmx5.coop.necrobat-platform-rider-admissions",1);
    cpu->gpr[2]=result;return 1;
}
static void necrobat_platform_reset_predicate(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[16];
    if(address!=0x800EFA54u || busy || !enrolled || failed ||
       (projected && !world_owner) || psx_mod_local_view_scope() || scene_owner ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=2 ||
       psx_mod_read_byte(PLAY+13) || psx_mod_read_word(0x800EE970u)!=6 ||
       !solid_record(actor) || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1) || psx_mod_read_byte(actor+2)!=5 ||
       psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+6)!=5 ||
       psx_mod_read_word(address)!=0x0064182Au ||
       psx_mod_read_word(0x800F44D4u)!=0x800EFA30u)return;
    int right=(int16_t)psx_mod_read_half(CAMERA+10)+416;
    if((int32_t)cpu->gpr[3]!=right)return;
    /* EFA54 resets the route when the platform is ahead of its one camera.
     * A rider's independent view must keep that same native route alive
     * while a living teammate waits behind. Keep the original reset only
     * when the support is ahead of every living view. */
    for(unsigned seat=0;seat<2;++seat)if(both_living(seat)) {
        int candidate=(int16_t)(le32(split_cameras[seat]+8)>>16)+416;
        if(candidate>right)right=candidate;
    }
    cpu->gpr[3]=(uint32_t)(int32_t)right;
}
static void route_solid_contact_predicate(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[16];
    if(projected || busy || !enrolled || failed || psx_mod_local_view_scope() ||
       scene_owner || !both_living(0) || !both_living(1) ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+13) ||
       !solid_record(actor) || !psx_mod_read_byte(actor))return;
    unsigned stage=psx_mod_read_byte(PLAY+12),reg,range=48;
    if(stage==7 && psx_mod_read_word(0x800EE970u)==14 &&
       psx_mod_read_word(0x800FC19Cu)==0x800F9654u &&
       psx_mod_read_word(0x800F99FCu)==0x0C00C903u &&
       psx_mod_read_word(0x800F9BE4u)==0x0C00C903u &&
       psx_mod_read_word(0x800F9D60u)==0x0C00C903u) {
        if((address==0x800F99D0u || address==0x800F9D34u) &&
           psx_mod_read_word(address)==0x00831023u)reg=4;
        else if(address==0x800F9BB8u && psx_mod_read_word(address)==0x00641023u)reg=3;
        else return;
    } else if(stage==5 && psx_mod_read_word(0x800EE970u)==11) {
        if(address==0x800F2D60u && psx_mod_read_word(address)==0x00641023u &&
           psx_mod_read_word(0x800F2CF8u)==0x27BDFFE8u &&
           psx_mod_read_word(0x800F2D8Cu)==0x0C00C903u) {reg=3;range=65;}
        else if(psx_mod_read_word(0x801016E4u)==0x800F851Cu &&
                psx_mod_read_word(0x800F88C4u)==0x0C00C903u &&
                psx_mod_read_word(0x800F8AACu)==0x0C00C903u &&
                psx_mod_read_word(0x800F8C28u)==0x0C00C903u) {
            if((address==0x800F8898u || address==0x800F8BFCu) &&
               psx_mod_read_word(address)==0x00831023u)reg=4;
            else if(address==0x800F8A80u && psx_mod_read_word(address)==0x00641023u)reg=3;
            else return;
        } else return;
    } else return;
    int x=(int16_t)psx_mod_read_half(actor+10);
    int first_x=(int16_t)psx_mod_read_half(PLAYER+10);
    int second_x=(int16_t)(both_body_word(1,8)>>16);
    int first=first_x-x,other=second_x-x;
    if(first<0)first=-first;
    if(other<0)other=-other;
    if((int16_t)cpu->gpr[reg]!=first_x || first<(int)range || other>=(int)range)return;
    /* These native solid callbacks skip3240C based on P1's distance before
     * the dual-body query can see P2. Admit a nearby second body; retain the
     * original collision, moving doors, crates and Gaea break conditions. */
    cpu->gpr[reg]=(uint32_t)(int32_t)second_x;
    psx_mod_counter_add(stage==7?"mmx5.coop.p2-rose-box-contact-admissions":
        "mmx5.coop.p2-kraken-solid-contact-admissions",1);
}
static void kraken_switch_retirement(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[16];
    if(address!=0x800F22D4u || busy || !enrolled || failed ||
       (projected && !world_owner) || psx_mod_local_view_scope() || scene_owner ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=5 ||
       psx_mod_read_byte(PLAY+13) || psx_mod_read_word(0x800EE970u)!=11 ||
       !solid_record(actor) || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=5 || psx_mod_read_byte(actor+4)!=1 ||
       (int16_t)psx_mod_read_half(actor+10)!=6584 ||
       psx_mod_read_word(address)!=0x14830002u ||
       psx_mod_read_word(0x800F22DCu)!=0xA6000088u ||
       cpu->gpr[3]!=2 || cpu->gpr[4]!=2)return;
    /* CP2 retires the first tube's purple switch by clearing its timer on
     * every update. Shared checkpoint progress does not mean the teammate
     * above that tube has crossed it. Retain its ordinary hit, timer and
     * closing program while a living hero is still on the upstream side:
     * the lower door ends at Y720, plus the native16px body half-height.
     * The next room's vertical doors begin at X6672. A fallen upstream hero
     * must not hold the switch, and a sole upstream survivor still needs it. */
    for(unsigned seat=0;seat<2;++seat)if(both_living(seat) &&
       (int16_t)(both_body_word(seat,8)>>16)<6672 &&
       (int16_t)(both_body_word(seat,12)>>16)<736) {
        cpu->gpr[3]=3; /* Skip only the checkpoint's forced zero at F22DC. */
        psx_mod_counter_add("mmx5.coop.kraken-upstream-switch-retained",1);
        return;
    }
}
static void falling_hazard_predicate(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[16];
    if(projected || busy || !enrolled || failed || psx_mod_local_view_scope() ||
       scene_owner || !both_living(0) || !both_living(1) ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=2 ||
       psx_mod_read_byte(PLAY+13)!=1 || psx_mod_read_word(0x800EE970u)!=7 ||
       actor<0x800C6A10u || actor>=0x800C9310u || (actor-0x800C6A10u)%0xA4u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x0F ||
       psx_mod_read_byte(actor+2)>1 || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_word(0x800F9340u)!=0x800F23E0u ||
       psx_mod_read_word(0x800F9CF8u)!=0x800F22C4u ||
       psx_mod_read_word(0x800F9D00u)!=0x800F244Cu)return;
    unsigned reg,radius;
    if(address==0x800F2474u && psx_mod_read_word(address)==0x00A31023u &&
       !psx_mod_read_byte(actor+6) && cpu->gpr[4]<=112 && !(cpu->gpr[4]&15u)) {
        reg=5;radius=cpu->gpr[4];
    }else if(address==0x800F2358u && psx_mod_read_word(address)==0x00831023u) {
        reg=4;radius=48;
    }else return;
    int x=(int16_t)psx_mod_read_half(actor+10);
    int first_x=(int16_t)cpu->gpr[reg];
    if((int16_t)cpu->gpr[3]!=x || first_x!=(int16_t)psx_mod_read_half(PLAYER+10))return;
    int first_distance=first_x-x;
    if(first_distance<0)first_distance=-first_distance;
    if((unsigned)first_distance<=radius)return;
    int second_x=(int16_t)(both_body_word(1,8)>>16),second_distance=second_x-x;
    if(second_distance<0)second_distance=-second_distance;
    if((unsigned)second_distance>radius)return;
    /* BIN3's native RNG-width trigger and its later48px contact gate both
     * read only PLAYER.x. Admit the other living hero without replaying RNG,
     * physics, effects or collision. The original3240C wrapper still owns
     * both bodies' real contacts; keep P1's native preference when eligible. */
    cpu->gpr[reg]=(uint32_t)(int32_t)second_x;
    psx_mod_counter_add(address==0x800F2474u?"mmx5.coop.p2-falling-hazard-triggers":
        "mmx5.coop.p2-falling-hazard-contact-predicates",1);
}
static void camera_emitter_origin(CPUState *cpu,uint32_t address) {
    if(!enrolled || failed || !split_views || !split_valid ||
       psx_mod_local_view_scope() || !camera_emitter_stage())return;
    uint32_t actor=cpu->gpr[16];
    if(address==0x800F3CA8u || address==0x800F3D60u) {
        if(!ai_call || ai_owner!=1 || !camera_emitter_parent(actor))return;
        /* The parent's native X follows the currently selected hero's
         * camera. Retain its original minus16 and one shared phase clock. */
        if(address==0x800F3CA8u)cpu->gpr[3]=camera_emitter_view(1,10);
        else cpu->gpr[2]=camera_emitter_view(1,10);
        return;
    }
    LandingEffect *effect=landing_effect(actor);
    if(!effect || effect->owner!=2 || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=0x14 || psx_mod_read_byte(actor+2)<3 ||
       psx_mod_read_byte(actor+2)>4 || psx_mod_read_byte(actor+4) ||
       !camera_emitter_parent(psx_mod_read_word(actor+0x50)))return;
    /* Use the allocating view through the deferred native initializer,
     * including its live-Y eligibility test. Keep RNG, original arithmetic,
     * resource upload, animation, movement and retirement untouched. */
    if(address==0x800F2874u)cpu->gpr[2]=(uint16_t)(camera_emitter_view(1,10)-16);
    else if(address==0x800F2978u)cpu->gpr[2]=(uint32_t)(int32_t)(int16_t)camera_emitter_view(1,14);
    else cpu->gpr[4]=camera_emitter_view(1,14);
}
static void disappearing_platform_predicate(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[16];
    if(address!=0x800F59A0u || projected || busy || !enrolled || failed ||
       psx_mod_local_view_scope() || scene_owner || !both_living(1) ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=12 ||
       psx_mod_read_byte(PLAY+13) || psx_mod_read_word(0x800EE970u)!=22 ||
       psx_mod_read_word(address)!=0x00A31023u ||
       psx_mod_read_word(0x800F70D8u)!=0x800F5864u ||
       psx_mod_read_word(0x800F7F88u)!=0x800F5990u ||
       actor<0x800C6A10u || actor>=0x800C9310u || (actor-0x800C6A10u)%0xA4u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x1B ||
       psx_mod_read_byte(actor+2) || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+6))return;
    int x=(int16_t)psx_mod_read_half(actor+10),first_x=(int16_t)cpu->gpr[5];
    if((int16_t)cpu->gpr[3]!=x || first_x!=(int16_t)psx_mod_read_half(PLAYER+10))return;
    int distance=first_x-x;
    if(distance<0)distance=-distance;
    if(distance<177)return;
    int second_x=(int16_t)(both_body_word(1,8)>>16);
    distance=second_x-x;
    if(distance<0)distance=-distance;
    if(distance>=177)return;
    /* Native F5990 tests only PLAYER.x. Admit the other living hero to
     * the same original60-update warning, animation, support and expiry.
     * Preserve the canonical P1 preference and update the parent once. */
    cpu->gpr[5]=(uint32_t)(int32_t)second_x;
    psx_mod_counter_add("mmx5.coop.p2-disappearing-platform-triggers",1);
}
static int descending_support_trigger(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(area_call || projected || busy || !enrolled || failed || psx_mod_local_view_scope() ||
       scene_owner || life.status[1]!=MMX5_COOP_ALIVE || second.body[4]!=1 ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=8 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=0x0Fu ||
       psx_mod_read_word(0x800F48B4u)!=0x3C03800Au ||
       psx_mod_read_word(0x800F48B8u)!=0x8462A0AAu ||
       psx_mod_read_word(0x800F9C6Cu)!=0x800F48B4u ||
       actor<0x800C6A10u || actor>=0x800C9310u || (actor-0x800C6A10u)%0xA4u ||
       psx_mod_read_byte(actor+1)!=0x12 || psx_mod_read_byte(actor+2) ||
       psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+5)!=2 || psx_mod_read_byte(actor+6))return 0;
    if(life.status[0]==MMX5_COOP_ALIVE && !support_pair_on_top(actor)) {
        /* Native F33E8 leaves this parent stationary in phase2 at its
         * authored boarding floor. F48B4 only admits its stand/walk command;
         * hold that predicate until both living heroes actually stand on
         * this support. F4808 still runs shots, drawing and both contacts. */
        psx_mod_counter_add("mmx5.coop.both-down-support-admission-waits",1);
        cpu->gpr[2]=0;return 1;
    }
    int first_x=(int16_t)psx_mod_read_half(PLAYER+10);
    int second_x=(int16_t)(second.body[10]|second.body[11]<<8);
    /* After both real top contacts, the trailing entrant still owns the
     * original walk to2121. A primary rider already at the centre must not
     * skip the other entrant's walk. Native P1 wins ties. */
    if(second_x<1944 || (life.status[0]==MMX5_COOP_ALIVE &&
       first_x>=1944 && first_x<=second_x))return 0;
    /* BIN11 F48B4 reads only PLAYER.x, then starts the original 1D/40
     * command and parent phase5. Project this admission once so script_begin
     * retains P2 as owner. F4808 still updates movement, shots and both
     * players' solid contacts exactly once on the canonical world pass. */
    area_call=1;enter_second();world_owner=1;
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    world_owner=0;leave_second_context(1);area_call=0;
    psx_mod_counter_add("mmx5.coop.p2-descending-support-triggers",1);
    cpu->gpr[2]=result;return 1;
}
static void up_support_resume_predicate(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[5];
    unsigned event=address==0x800F4CE4u?1u:address==0x800F4D1Cu?2u:0u;
    if(!event || !enrolled || failed || psx_mod_local_view_scope() ||
       life.status[0]!=MMX5_COOP_ALIVE || life.status[1]!=MMX5_COOP_ALIVE ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=8 ||
       psx_mod_read_byte(PLAY+13) || psx_mod_read_word(0x800EE970u)!=0x0Fu ||
       psx_mod_read_word(address)!=0x14830007u ||
       psx_mod_read_word(0x800F9C9Cu+event*8u)!=(event==1?0x800F4CD4u:0x800F4D0Cu) ||
       !solid_record(actor) || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=0x12 || psx_mod_read_byte(actor+2)!=2 ||
       psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+5)!=7 ||
       psx_mod_read_byte(actor+6)!=event*2u || cpu->gpr[4]!=event ||
       support_pair_on_top(actor))return;
    /* Pre-branch hook: native LB and its load-delay slot have completed.
     * Native subtype5/6 bomb retirement has published PLAY26=1/2. Preserve
     * that event while a living partner returns to either stopped segment.
     * Only the original event operand waits; the parent still updates/draws/
     * collides, and F4CEC/F4D24 owns the120/30 timers after real reboarding. */
    cpu->gpr[4]=0;
}
static uint32_t later_support_as_second(CPUState *cpu,uint32_t address,uint32_t actor) {
    uint8_t camera[sizeof saved_camera];unsigned camera_owner=split_owner;
    int authored=pegasus_platform_camera();
    int own_camera=split_views && split_valid && !authored;
    if(own_camera) {
        capture(CAMERA,camera,sizeof camera);project(CAMERA,split_cameras[1],sizeof camera);split_owner=2;
    }
    uint8_t campaign=psx_mod_read_byte(PLAY+0x44);
    enter_second();world_owner=1;
    psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    world_owner=0;leave_second_context(1);
    if(own_camera) {
        capture(CAMERA,split_cameras[1],sizeof camera);
        /* Admission may begin the authored sequence in this very call.
         * Its native camera command becomes canonical, just as subsequent
         * travel does, rather than being discarded by the private restore. */
        if(!pegasus_platform_camera() && scene_owner!=2) {
            project(CAMERA,camera,sizeof camera);split_owner=camera_owner;
        }
    }
    if(split_views && split_valid && (authored || pegasus_platform_camera())) {
        /* Include the last real travel update when native phase8 releases
         * the sequence, then ordinary private views resume from that result. */
        uint8_t command[sizeof saved_camera];capture(CAMERA,command,sizeof command);
        pegasus_camera_command(command);
    }
    return result;
}
static int glow_teleport_admission(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];SolidHistory *h=solid_record(actor);
    int resident_second=projected && world_owner;
    if(area_call || busy || (projected && !world_owner) || !h || !split_available() || scene_owner ||
       psx_mod_local_view_scope() || life.status[1]!=MMX5_COOP_ALIVE ||
       second.body[4]!=1 || !(second.body[0x5C]&127u) ||
       psx_mod_read_byte(PLAY+12)!=6 || psx_mod_read_byte(PLAY+13)!=1 ||
       psx_mod_read_word(0x800EE970u)!=13 ||
       psx_mod_read_word(0x800FB28Cu)!=0x800F7368u ||
       psx_mod_read_word(0x800FB29Cu)!=address ||
       psx_mod_read_word(address)!=0x27BDFFE0u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=14 ||
       (psx_mod_read_byte(actor+2)&0x80u) || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+5))return 0;
    if(!resident_second && ((psx_mod_read_byte(actor+0x72)&8u) || !(h->sides&8u)))return 0;
    /* Native F7618 admits the sender from its previous top contact, then
     * snaps/stands that hero and runs the original beam/checkpoint program.
     * The shared actor retains P1's contact; P2's real contact is in h.
     * Project only this admission leaf, leaving the parent animation and
     * dual-player solid pass to run once in their original order. When P1
     * has fallen, the world already projects P2, but solid_as_second still
     * restores P1's contact on the shared actor. Borrow P2's history there
     * too, without entering a second projection or accepting stale P1 top. */
    unsigned sides=psx_mod_read_byte(actor+0x72);
    psx_mod_write_byte(actor+0x72,h->sides);area_call=1;
    uint32_t result=resident_second?guest(cpu,address,actor,cpu->gpr[5]):
        later_support_as_second(cpu,address,actor);
    area_call=0;psx_mod_write_byte(actor+0x72,(uint8_t)sides);
    if(h->sides&8u)psx_mod_counter_add("mmx5.coop.p2-glow-teleport-admissions",1);
    cpu->gpr[2]=result;return 1;
}
static int later_support_waiting_for_riders(uint32_t actor,const SolidHistory *h) {
    /* These are the two native top-contact histories for this physical
     * support, not the players' generic grounded flags or proximity. A side
     * touch must not admit a lift, and an absent/dead partner cannot veto it. */
    return life.status[0]==MMX5_COOP_ALIVE && life.status[1]==MMX5_COOP_ALIVE &&
        (!(psx_mod_read_byte(actor+0x72)&8u) || !(h->sides&8u));
}
static void ascending_support_predicate(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];SolidHistory *h=solid_record(actor);
    if(address!=0x800F4990u || !h || !split_available() ||
       psx_mod_read_byte(PLAY+12)!=8 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=0x0Fu ||
       psx_mod_read_word(0x800F4968u)!=0x27BDFFE0u ||
       psx_mod_read_word(address)!=0x28420E21u ||
       psx_mod_read_word(0x800F9C74u)!=0x800F4968u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x12 ||
       psx_mod_read_byte(actor+2)!=2 || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+5)!=4 || !later_support_waiting_for_riders(actor,h))return;
    /* Keep F4968's ordinary per-frame top clamp, but leave its original
     * >=3617 departure comparison closed until both living riders board.
     * The native motion, camera command, sounds and carry still run once. */
    cpu->gpr[2]=0;
    psx_mod_counter_add("mmx5.coop.both-later-support-waits",1);
}
static int later_support_controller(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];SolidHistory *h=solid_record(actor);
    if(area_call || busy || !enrolled || failed || psx_mod_local_view_scope() || scene_owner ||
       (projected && !world_owner) || !h || psx_mod_read_byte(PLAY)!=10 ||
       psx_mod_read_byte(PLAY+12)!=8 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=0x0Fu || psx_mod_read_byte(actor+1)!=0x12 ||
       psx_mod_read_byte(actor+4)!=1)return 0;
    unsigned subtype=psx_mod_read_byte(actor+2),phase=psx_mod_read_byte(actor+5);
    if(address==0x800F48F8u) {
        if(subtype!=1 || phase!=3 || psx_mod_read_word(address)!=0x3C03800Du ||
           psx_mod_read_word(address+4)!=0x80621C29u || psx_mod_read_word(0x800F9C70u)!=address)return 0;
        if(later_support_waiting_for_riders(actor,h)) {
            /* Preserve the idle callback and its ordinary sound request.
             * Mask only its previous-contact admission while a living hero
             * is still off this lift; no map change or motion begins early. */
            uint8_t sides=psx_mod_read_byte(actor+0x72);
            psx_mod_write_byte(actor+0x72,0);area_call=1;
            uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);area_call=0;
            psx_mod_write_byte(actor+0x72,sides);h->controller_p2=0;
            psx_mod_counter_add("mmx5.coop.both-later-support-waits",1);
            cpu->gpr[2]=result;return 1;
        }
        if(!projected && life.status[0]==MMX5_COOP_ALIVE && (psx_mod_read_byte(actor+0x72)&0x0B)) {
            h->controller_p2=0;return 0;
        }
        if(life.status[1]!=MMX5_COOP_ALIVE || (!projected && !(h->sides&0x0B)))return 0;
        /* F48F8 reads the previous native solid contact, not PLAYER. Give it
         * P2's actual history even in an already-projected survivor pass.
         * Its SFX/quake/phase update remains one original call. */
        uint8_t sides=psx_mod_read_byte(actor+0x72);
        psx_mod_write_byte(actor+0x72,h->sides);area_call=1;
        uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);area_call=0;
        psx_mod_write_byte(actor+0x72,sides);
        h->controller_p2=psx_mod_read_byte(actor+5)==6;
        if(h->controller_p2)psx_mod_counter_add("mmx5.coop.p2-later-support-admissions",1);
        cpu->gpr[2]=result;return 1;
    }
    if(projected)return 0; /* The survivor world already supplies P2. */
    if(address==0x800F4968u) {
        if(subtype!=2 || phase!=4 || psx_mod_read_word(address)!=0x27BDFFE0u ||
           psx_mod_read_word(address+4)!=0xAFB20018u || psx_mod_read_word(0x800F9C74u)!=address)return 0;
        if(life.status[0]==MMX5_COOP_ALIVE && (int16_t)psx_mod_read_half(PLAYER+10)>=3617) {
            h->controller_p2=0;return 0;
        }
        if((int16_t)(second.body[10]|second.body[11]<<8)<3617)return 0;
    } else if(address==0x800F3930u) {
        if(subtype!=1 || phase!=6 || psx_mod_read_word(address)!=0x27BDFFE0u ||
           psx_mod_read_word(0x800F9C7Cu)!=address || !h->controller_p2)return 0;
    } else if(address==0x800F4A5Cu) {
        if(subtype!=2 || phase!=7 || psx_mod_read_word(address)!=0x27BDFFE8u ||
           psx_mod_read_word(0x800F9C80u)!=address)return 0;
        if(!h->controller_p2) {
            /* P1's original callback needs the same final camera publication
             * as the P2 adapter when movement enters a stationary stop. Keep
             * its last native Y/bounds in both views before private X resumes;
             * actual resumed movement publishes its native command again. */
            int authored=pegasus_platform_camera();area_call=1;
            uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);area_call=0;
            if(split_views && split_valid && (authored || pegasus_platform_camera())) {
                uint8_t command[sizeof saved_camera];capture(CAMERA,command,sizeof command);
                pegasus_camera_command(command);
            }
            cpu->gpr[2]=result;return 1;
        }
    } else return 0;
    if(life.status[1]!=MMX5_COOP_ALIVE || second.body[4]!=1) {h->controller_p2=0;return 0;}
    /* Retain the admitting seat through the original motion substates. Only
     * these callbacks see its body/camera: F4808 keeps canonical combat and
     * dual-player contacts, and the shared parent still advances once. */
    area_call=1;uint32_t result=later_support_as_second(cpu,address,actor);area_call=0;
    unsigned next=psx_mod_read_byte(actor+5);
    h->controller_p2=(subtype==1?next==6:next==7);
    psx_mod_counter_add("mmx5.coop.p2-later-support-controller-ticks",1);
    if(address==0x800F4968u && h->controller_p2)psx_mod_counter_add("mmx5.coop.p2-later-support-admissions",1);
    cpu->gpr[2]=result;return 1;
}
static unsigned field_recipient(uint32_t actor,unsigned target) {
    if(projected || psx_mod_read_byte(PLAY+12)!=2 || psx_mod_read_byte(PLAY+13)!=1 ||
       psx_mod_read_word(0x800EE970u)!=7 || psx_mod_read_byte(actor+1)!=0x39 ||
       psx_mod_read_byte(actor+2) || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+5)!=9 || psx_mod_read_byte(actor+6)<4 ||
       psx_mod_read_byte(actor+6)>7 ||
       psx_mod_read_word(0x800FA11Cu)!=0x800F7F44u ||
       psx_mod_read_word(0x800FA1B0u)!=0x800F7F98u ||
       psx_mod_read_word(0x800FA1C0u)!=0x800F8084u)return target;
    int first_held=life.status[0]==MMX5_COOP_ALIVE && psx_mod_read_byte(PLAYER+0xF7);
    int second_held=life.status[1]==MMX5_COOP_ALIVE && second.body[0xF7];
    if(first_held==second_held)return target;
    /* Dark Necrobat's native field sets its recipient's F7 at F8050, then
     * clears that same player's flag at F80C8. Re-selecting the nearest hero
     * during the intervening shared substates can clear the other body and
     * leave the original recipient unable to use weapons after the field.
     * The existing native flag identifies the one held living recipient;
     * it is already part of each serialized body. Preserve native admission,
     * timing, global effects and release without adding another owner state. */
    unsigned recipient=second_held?1u:0u;
    if(target!=recipient)psx_mod_counter_add("mmx5.coop.field-recipient-targets",1);
    return recipient;
}
static unsigned grab_recipient(CPUState *cpu,uint32_t actor,unsigned target) {
    unsigned family=rose_capture_actor(actor);
    if(projected || !family)return target;
    HitHistory *capture=hit_record(actor,0);
    if(capture && capture->capture_owner &&
       life.status[capture->capture_owner-1]==MMX5_COOP_ALIVE) {
        /* Keep this actual acquisition through periodic injury, escape and
         * parent/attack retirement. Other hands can retain their own victim;
         * proximity and another held body's flags cannot transfer ownership. */
        unsigned recipient=capture->capture_owner-1;
        if(target!=recipient)psx_mod_counter_add("mmx5.coop.grab-recipient-targets",1);
        return recipient;
    }
    int admission=family==1?
        psx_mod_read_byte(actor+4)==2 && psx_mod_read_byte(actor+5)==2:
        psx_mod_read_byte(actor+4)==1 && !psx_mod_read_byte(actor+5);
    if(admission && psx_mod_read_word(actor+0x50)) {
        /* A long whip's contact box can lie beside a different hero from
         * its anchor. Prefer an actual native overlap to anchor proximity;
         * these read-only geometry queries do not apply damage or capture. */
        unsigned overlap[2]={0,0};
        if(life.status[0]==MMX5_COOP_ALIVE && !scene_passenger(0) &&
           !psx_mod_read_byte(PLAYER+0xCC))
            overlap[0]=guest(cpu,0x8002E274u,actor,PLAYER)!=0;
        if(life.status[1]==MMX5_COOP_ALIVE && !scene_passenger(1) && !second.body[0xCC]) {
            enter_second();overlap[1]=guest(cpu,0x8002E274u,actor,PLAYER)!=0;leave_second();
        }
        if(overlap[0]!=overlap[1])return overlap[1]?1u:0u;
    }
    if(family!=2 || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+5)!=1)return target;
    int first_ca=(int8_t)psx_mod_read_byte(PLAYER+0xCA);
    int second_ca=(int8_t)second.body[0xCA];
    int first_held=life.status[0]==MMX5_COOP_ALIVE &&
        psx_mod_read_byte(PLAYER+0x63)==3 && (first_ca>0 ||
        (first_ca<0 && psx_mod_read_byte(PLAYER+4)==1 && psx_mod_read_byte(PLAYER+5)==0x11));
    int second_held=life.status[1]==MMX5_COOP_ALIVE && second.body[0x63]==3 &&
        (second_ca>0 || (second_ca<0 && second.body[4]==1 && second.body[5]==0x11));
    if(first_held==second_held)return target;
    /* Rosered's held child uses PLAYER for its periodic damage, escape input
     * and CA release. CA cycles through negative damage commands while the
     * victim retains reaction3; ordinary reaction1 damage is not a capture.
     * Pin only one unambiguous native grab/victim pair. Multiple held children
     * need their own association and must not inherit another child's victim. */
    unsigned held_children=0;
    for(unsigned i=0;i<32;++i) {
        uint32_t child=0x80096D98u+i*0x9Cu;
        if(psx_mod_read_byte(child) && psx_mod_read_byte(child+1)==0x27 &&
           !psx_mod_read_byte(child+2) && psx_mod_read_byte(child+4)==1 &&
           psx_mod_read_byte(child+5)==1)++held_children;
    }
    if(held_children!=1)return target;
    unsigned recipient=second_held?1u:0u;
    if(target!=recipient)psx_mod_counter_add("mmx5.coop.grab-recipient-targets",1);
    return recipient;
}
static int grizzly_stage(void) {
    return psx_mod_read_byte(PLAY)==10 && psx_mod_read_byte(PLAY+12)==1 &&
        !psx_mod_read_byte(PLAY+13) && psx_mod_read_word(0x800EE970u)==5u &&
        psx_mod_read_word(0x800F597Cu)==0xA4620024u &&
        psx_mod_read_word(0x800F59FCu)==0xA6230024u &&
        psx_mod_read_word(0x800EF3ACu)==0x27BDFFE8u &&
        psx_mod_read_word(0x800FBFBCu)==0x800EF424u;
}
static int dinorex_fire_content(void) {
    return psx_mod_read_byte(PLAY)==10 && psx_mod_read_byte(PLAY+12)==4 &&
        !psx_mod_read_byte(PLAY+13) && psx_mod_read_word(0x800EE970u)==9u &&
        psx_mod_read_word(0x800F3C48u)==0x8462A0AAu &&
        psx_mod_read_word(0x800F3C58u)==0x28420471u &&
        psx_mod_read_word(0x800F58F8u)==0x800F3B54u &&
        psx_mod_read_word(0x800F5804u)==0x800F25BCu;
}
static int dinorex_fire_stage(void) {
    return split_available() && dinorex_fire_content();
}
static int dinorex_fire_view_region(unsigned seat,int *x) {
    if(seat>1 || !split_valid || !dinorex_sequence.owner || dinorex_sequence.owner>2)return 0;
    int y=(int16_t)camera_emitter_view(seat,0x1A);
    /* The upper approach already shares the native warning and fire cycle.
     * Rebase its band along with the lower corridor so a second, deeper
     * camera cannot move the first view's image away from its contact band.
     * CB48 admits that room with maxY2416; its192px native focus can put
     * the actual origin above2416, including while jumping or shaking.
     * F29B4 still owns the actual corner/terrain/cover checks. */
    if(y<2192 || y>=3136 || (int16_t)camera_emitter_view(seat,0x28)<2416 ||
       (int16_t)(both_body_word(seat,8)>>16)<1137)return 0;
    *x=(int16_t)(split_cameras[seat][10]|split_cameras[seat][11]<<8);return 1;
}
static uint32_t dinorex_fire_contact_view(CPUState *cpu,uint32_t address,unsigned seat,int reference_x) {
    uint8_t band[4];capture(0x800F5960u,band,sizeof band);
    int x;
    if(dinorex_fire_view_region(seat,&x)) {
        int delta=x-reference_x;
        for(unsigned at=0;at<4;at+=2)
            psx_mod_write_half(0x800F5960u+at,
                (uint16_t)((band[at]|band[at+1]<<8)+delta));
    }
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    project(0x800F5960u,band,sizeof band);return result;
}
static int dinorex_fire_contact_canonical(CPUState *cpu,uint32_t address) {
    if(contact_call)return 0;
    contact_call=1;
    unsigned resident=world_owner?1u:0u;
    /* Both native sweep/hold leaves publish left = foreground X - 64.
     * Derive that tick's reference from the command itself, including a
     * survivor world that updates the original AI without a retained scope. */
    int reference_x=(int16_t)psx_mod_read_half(0x800F5960u)+64;
    uint32_t results[2]={0,0};
    if(life.status[resident]==MMX5_COOP_ALIVE &&
       psx_mod_read_byte(PLAYER+4)==1 &&
       (psx_mod_read_byte(PLAYER+0x5C)&127u))
        results[resident]=dinorex_fire_contact_view(cpu,address,resident,reference_x);
    if(!projected && !world_owner && life.status[1]==MMX5_COOP_ALIVE &&
       second.body[4]==1 && (second.body[0x5C]&127u)) {
        enter_second();
        results[1]=dinorex_fire_contact_view(cpu,address,1,reference_x);
        leave_second();
    }
    contact_call=0;
    cpu->gpr[2]=results[ai_call?ai_owner:resident];return 1;
}
static int dinorex_fire_contact(CPUState *cpu,uint32_t address) {
    if(contact_call || !enrolled || failed || psx_mod_local_view_scope() ||
       !dinorex_fire_content() || address!=0x800F2AE8u ||
       psx_mod_read_word(address)!=0x27BDFFE0u ||
       psx_mod_read_word(0x800F2B14u)!=0x0C00BED6u ||
       psx_mod_read_word(0x800F2AB0u)!=0xA244005Cu)return 0;
    /* F302C/F3338 publish the shared fire band before calling this leaf.
     * Its original four-corner terrain/cover checks and fatal HP/accounting
     * apply to each live body, even when the burst's AI belongs to P2.
     * Do not tick the burst, particles, sound or its camera a second time. */
    return ai_canonical_call(cpu,address,dinorex_fire_contact_canonical);
}
static void section_exit_predicate(CPUState *cpu,uint32_t address) {
    if(projected || !split_available() || scene_owner ||
       psx_mod_read_byte(PLAY+13) || life.status[1]!=MMX5_COOP_ALIVE ||
       second.body[4]!=1 || !(second.body[0x5C]&127u))return;
    int izzy=address==0x800F26D4u && psx_mod_read_byte(PLAY+12)==6 &&
        psx_mod_read_word(0x800EE970u)==12u &&
        psx_mod_read_word(0x800F26C8u)==0x3C03800Au &&
        psx_mod_read_word(0x800F26CCu)==0x8462A0AAu &&
        psx_mod_read_word(address)==0x28420EB1u &&
        psx_mod_read_word(0x800F26ECu)==0xA0621C0Fu;
    int dinorex=address==0x800F39E8u && dinorex_fire_content() &&
        psx_mod_read_word(0x800F39BCu)==0x27BDFFE8u &&
        psx_mod_read_word(address)==0x28421134u &&
        psx_mod_read_word(0x800F39FCu)==0xA0621C0Fu &&
        psx_mod_read_word(0x800F5818u)==0x800F39BCu;
    if(!izzy && !dinorex)return;
    int first_x=(int16_t)psx_mod_read_half(PLAYER+10);
    int second_x=(int16_t)(le32(second.body+8)>>16);
    if((int32_t)cpu->gpr[2]!=first_x || second_x<=first_x)return;
    /* These native post-barrier leaves compare the resident PLAYER.x only.
     * Extend that one predicate to the other living hero. Do not replay the
     * controller or rely on a0 (neither leaf takes an actor argument), and
     * retain the actual destruction/explosion phase, Dinorex HP test and
     * original2D720 exit/continuation request. Survivor world passes already
     * have the living resident and keep their native comparison unchanged. */
    cpu->gpr[2]=(uint32_t)(int32_t)second_x;
    psx_mod_counter_add(izzy?"mmx5.coop.p2-izzy-section-predicates":
                            "mmx5.coop.p2-dinorex-section-predicates",1);
}
static int dinorex_wall_camera(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(area_call || busy || projected || !dinorex_fire_stage() ||
       address!=0x800F3780u || !solid_record(actor) ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x0C ||
       psx_mod_read_byte(actor+2)!=1 ||
       psx_mod_read_word(address)!=0x27BDFFE8u ||
       psx_mod_read_word(0x800F379Cu)!=0x2463580Cu ||
       psx_mod_read_word(0x800F580Cu)!=0x800F2378u)return 0;
    /* Native F2378/F25BC belongs to terrain class0C/sub1, not enemy24.
     * Its one controller captures/restores camera bounds and waits for the
     * actual fire destruction. Keep it on the burst's admitting camera so
     * a leading P2 can reach the native stop and release all three walls. */
    Mmx5CoopViewActor actors[2];split_actors(actors);
    unsigned target=mmx5_coop_view_nearest(actors,
        (int16_t)psx_mod_read_half(actor+10),(int16_t)psx_mod_read_half(actor+14),0);
    /* F24C8/F24D8 saved this admitted camera's bounds in the wall. Its
     * F2770/F2780 release must restore that same camera after an owner fatal,
     * rather than applying the lower route's saved bounds to the survivor. */
    if(dinorex_sequence.owner && dinorex_sequence.owner<=2)target=dinorex_sequence.owner-1;
    int stop=(int16_t)psx_mod_read_half(actor+10)-304;
    unsigned phase=psx_mod_read_byte(actor+4);
    area_call=1;
    uint32_t result=target?later_support_as_second(cpu,address,actor):
        guest(cpu,address,actor,cpu->gpr[5]);
    area_call=0;
    /* The wall's native substate advances once, but its camera stop belongs
     * to both lower-route views. A trailing pane has not reached the stop
     * when the shared phase6 latches; it still needs the same right limit.
     * Conversely, retirement must release every admitted pane, not only the
     * current fire owner. The matching stop identifies the admitted panes;
     * reuse the wall's original saved corridor limits without another latch. */
    int active=psx_mod_read_byte(actor) && psx_mod_read_byte(actor+4)==1 &&
        psx_mod_read_byte(actor+5)==1;
    if(active)for(unsigned seat=0;seat<2;++seat)if(both_living(seat)) {
        uint8_t *view=split_cameras[seat];
        int top=(int16_t)(le32(view+12)>>16);
        int minimum=(int16_t)camera_emitter_view(seat,0x26);
        if(top<2416 || top>=3136 || minimum>stop)continue;
        both_half(view,0x24,stop);
        if((int16_t)(le32(view+8)>>16)>=stop)both_half(view,0x26,stop);
    }
    if(!active && phase==1) {
        for(unsigned seat=0;seat<2;++seat) {
            uint8_t *view=split_cameras[seat];
            if((int16_t)camera_emitter_view(seat,0x24)==stop)
                both_half(view,0x24,psx_mod_read_half(actor+0x7E));
            if((int16_t)camera_emitter_view(seat,0x26)==stop)
                both_half(view,0x26,psx_mod_read_half(actor+0x7C));
        }
        psx_mod_counter_add("mmx5.coop.dinorex-wall-view-releases",1);
    }
    unsigned camera=split_owner?split_owner-1:world_owner;
    psx_mod_write_half(CAMERA+0x24,camera_emitter_view(camera,0x24));
    psx_mod_write_half(CAMERA+0x26,camera_emitter_view(camera,0x26));
    cpu->gpr[2]=result;
    if(target)psx_mod_counter_add("mmx5.coop.p2-dinorex-native-wall-ticks",1);
    return 1;
}
static int dinorex_upper_wall_camera(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(area_call || busy || (projected && !world_owner) || !split_available() ||
       psx_mod_read_byte(PLAY+12)!=4 || psx_mod_read_byte(PLAY+13)!=1 ||
       !native_armor_overlay() || address!=0x800F66D4u ||
       psx_mod_read_word(address)!=0x27BDFFE8u ||
       psx_mod_read_word(0x800FEEA0u)!=address || !solid_record(actor) ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x0B ||
       psx_mod_read_byte(actor+2)<8 || psx_mod_read_byte(actor+2)>10)return 0;
    unsigned phase=psx_mod_read_byte(actor+4),subtype=psx_mod_read_byte(actor+2);
    int stop=subtype==8?1264:subtype==9?1904:2560;
    int old_min=(int16_t)psx_mod_read_half(CAMERA+0x26);
    uint32_t box=psx_mod_read_word(actor+0x68);
    int bottom=(int16_t)psx_mod_read_half(actor+14);
    if(box>=0x80010000u && box<0x80200000u)
        bottom+=(int8_t)psx_mod_read_byte(box+1)+psx_mod_read_byte(box+3);
    area_call=1;uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);area_call=0;
    int maximum=(int16_t)psx_mod_read_half(CAMERA+0x24);
    /* F67C4 locks the upper route; F652C releases that lock and advances
     * its left edge after real destruction. These commands belong to each
     * admitted view, not the singleton world camera. A hero still beside
     * the starting suit must not inherit the leading hero's forward clamp.
     * Run the original rock, damage and retirement exactly once. */
    int lock=(phase==0 || phase==2) && maximum==stop;
    int release=phase==3 && (int16_t)psx_mod_read_half(CAMERA+0x26)==stop;
    if(lock || release) {
        for(unsigned seat=0;seat<2;++seat)if(both_living(seat)) {
            uint8_t *view=split_cameras[seat];
            int top=(int16_t)(le32(view+12)>>16);
            int minimum=(int16_t)camera_emitter_view(seat,0x26);
            int previous_max=(int16_t)camera_emitter_view(seat,0x24);
            int x=(int16_t)(both_body_word(seat,8)>>16);
            if((lock && top<=bottom && minimum<=stop) ||
               (release && previous_max==stop))both_half(view,0x24,maximum);
            if(release && top<=bottom && x>=stop+8 && minimum<stop)
                both_half(view,0x26,stop);
        }
        unsigned owner=split_owner?split_owner-1:world_owner;
        psx_mod_write_half(CAMERA+0x24,camera_emitter_view(owner,0x24));
        psx_mod_write_half(CAMERA+0x26,release?
            camera_emitter_view(owner,0x26):(uint16_t)old_min);
        psx_mod_counter_add("mmx5.coop.dinorex-upper-wall-camera-commands",1);
    }
    cpu->gpr[2]=result;return 1;
}
static int dinorex_lava_camera(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(area_call || busy || projected || !split_available() ||
       psx_mod_read_byte(PLAY+12)!=4 || psx_mod_read_byte(PLAY+13)!=1 ||
       psx_mod_read_byte(PLAY+0x1D) || !native_armor_overlay() ||
       address!=0x800F6B60u || psx_mod_read_word(address)!=0x27BDFFE0u ||
       psx_mod_read_word(0x800FEDE8u)!=address ||
       psx_mod_read_word(0x800F6C20u)!=0x0C03DA9Fu ||
       actor<0x8009E7A0u || actor>=0x8009F9A0u || (actor-0x8009E7A0u)%0x30u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x0A)return 0;
    area_call=1;uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    both_half(split_cameras[0],0x2A,(int16_t)psx_mod_read_half(CAMERA+0x2A));
    if(both_living(1)) {
        /* This area leaf has no world/animation clock: it checks the native
         * six lava regions (F6A7C), applies fatal HP/rank accounting, and
         * selects this body's sloped camera floor. Evaluate that same leaf
         * for P2 on its own camera. Retain the one area actor's committed
         * initialization and mount/pose cache from the original P1 pass. */
        uint8_t state[0x30];capture(actor,state,sizeof state);
        enter_second();project(CAMERA,split_cameras[1],sizeof saved_camera);
        guest(cpu,address,actor,cpu->gpr[5]);
        both_half(split_cameras[1],0x2A,(int16_t)psx_mod_read_half(CAMERA+0x2A));
        leave_second();project(actor,state,sizeof state);
        psx_mod_counter_add("mmx5.coop.p2-dinorex-native-lava-checks",1);
    }
    area_call=0;cpu->gpr[2]=result;return 1;
}
static int dinorex_fire_actor(uint32_t actor) {
    return psx_mod_read_byte(actor+1)==0x25 && !psx_mod_read_byte(actor+2);
}
static int sigma1_stage(void) {
    return enrolled && !failed && psx_mod_read_byte(PLAY)==10 &&
        psx_mod_read_byte(PLAY+12)==16 && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==30u &&
        psx_mod_read_word(0x800EF360u)==0x0C00A2C6u &&
        psx_mod_read_word(0x800EF388u)==0xAC4492F8u;
}
static int sigma1_floor_admission(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(address!=0x800EF308u || area_call || busy || projected ||
       !split_available() || !sigma1_stage() || scene_owner ||
       actor<0x8009E7A0u || actor>=0x8009F9A0u || (actor-0x8009E7A0u)%0x30u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=13 ||
       psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+2)>1 || !both_living(1))return 0;
    uint32_t param=0x800F8C4Cu+psx_mod_read_byte(actor+2)*14u;
    if(psx_mod_read_word(actor+0x18)!=param)return 0;
    int threshold=(int16_t)psx_mod_read_half(param+12);
    if((both_living(0) && (int16_t)psx_mod_read_half(CAMERA+14)>=threshold &&
        (int16_t)psx_mod_read_half(PLAYER+14)>=threshold+48) ||
       (int16_t)camera_emitter_view(1,14)<threshold ||
       (int16_t)(both_body_word(1,12)>>16)<threshold+48)return 0;
    /* Admit the shared scenery/checkpoint through the same native leaf
     * when P2 leads. Only its floor copy waits for the living follower. */
    area_call=1;uint32_t result=later_support_as_second(cpu,address,actor);area_call=0;
    cpu->gpr[2]=result;return 1;
}
static int sigma1_floor_retire(CPUState *cpu,uint32_t address) {
    static unsigned calling;
    uint32_t actor=cpu->gpr[4];
    if(calling || address!=0x800EF3ACu || !sigma1_stage() ||
       psx_mod_local_view_scope() || actor<0x8009E7A0u || actor>=0x8009F9A0u ||
       (actor-0x8009E7A0u)%0x30u || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=13 || psx_mod_read_byte(actor+4)!=2 ||
       psx_mod_read_byte(actor+2)>1)return 0;
    uint32_t param=0x800F8C4Cu+psx_mod_read_byte(actor+2)*14u;
    if(psx_mod_read_word(actor+0x18)!=param ||
       psx_mod_read_word(0x800EF3B4u)!=0x0C00B5C8u)return 0;
    /* F308 already admitted the leader's native checkpoint/scenery request.
     * Its phase2 actor retains the original floor descriptor until the last
     * living hero is below that row. No extra timer or snapshot state. */
    int threshold=(int16_t)psx_mod_read_half(param+12)+48;
    for(unsigned seat=0;seat<2;++seat)if(both_living(seat) &&
       (int16_t)(both_body_word(seat,12)>>16)<threshold) {
        cpu->gpr[2]=0;return 1;
    }
    calling=1;
    uint32_t a2=cpu->gpr[6],a3=cpu->gpr[7];
    cpu->gpr[6]=psx_mod_read_half(param+2);cpu->gpr[7]=param+4;
    guest(cpu,0x80028B18u,0,psx_mod_read_half(param));
    cpu->gpr[6]=a2;cpu->gpr[7]=a3;
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);calling=0;
    psx_mod_counter_add("mmx5.coop.sigma1-completed-floor-closures",1);
    cpu->gpr[2]=result;return 1;
}
static int sigma1_fixed_emitter(uint32_t parent) {
    if(!sigma1_stage())return 0;
    if(parent<0x80092090u || parent>=0x80093DD0u || (parent-0x80092090u)%0x9Cu ||
       !psx_mod_read_byte(parent) || psx_mod_read_byte(parent+1)!=0x29 ||
       psx_mod_read_byte(parent+2)>3 ||
       psx_mod_read_word(0x800FA48Cu)!=0x800F7124u)return 0;
    uint32_t record=psx_mod_read_word(parent+0x10);
    if(record<0x800F892Cu || (record-0x800F892Cu)%8u || record>=0x800F8C4Cu ||
       psx_mod_read_byte(record+1)!=0x29 ||
       psx_mod_read_byte(record+2)!=psx_mod_read_byte(parent+2))return 0;
    return 1;
}
static int sigma1_laser_proximity(CPUState *cpu,uint32_t address) {
    (void)address;uint32_t parent=cpu->gpr[4];
    if(!split_available() || !sigma1_fixed_emitter(parent))return 0;
    unsigned axis=psx_mod_read_byte(parent+2)<2?14u:10u;
    unsigned across_axis=axis==14?10u:14u;
    for(unsigned seat=0;seat<2;++seat)if(both_living(seat)) {
        int along=(int16_t)(both_body_word(seat,axis-2u)>>16)-
            (int16_t)psx_mod_read_half(parent+axis);
        int across=(int16_t)(both_body_word(seat,across_axis-2u)>>16)-
            (int16_t)psx_mod_read_half(parent+across_axis);
        /* The native admission remains16px. Keep its active attempt only
         * near a living hero;96px ends before the last door's physical stop.
         * Fatal HP is excluded even before the lifecycle publishes death. */
        if(along>=-32 && along<96 && across>=-336 && across<=336)return 0;
    }
    if(!psx_mod_read_byte(parent+5) && !psx_mod_read_byte(parent+6))return 0;
    /* F69C8 can admit the next hero normally after this narrow window.
     * Retire both native child pools, leaving the emitter and placement
     * intact. No dead body, old view or persistent latch keeps firing. */
    for(uint32_t child=0x80096D98u;child<0x80098118u;child+=0x9Cu)
        if(psx_mod_read_byte(child) && psx_mod_read_byte(child+1)==0x20 &&
           psx_mod_read_word(child+0x7C)==parent)
            guest(cpu,0x8002D6FCu,child,0);
    for(uint32_t child=0x800D1F40u;child<0x800D4F40u;child+=0x60u)
        if(psx_mod_read_byte(child) && psx_mod_read_byte(child+1)==0x29 &&
           psx_mod_read_word(child+0x50)==parent)
            guest(cpu,0x8002D6FCu,child,0);
    psx_mod_write_byte(parent+5,0);psx_mod_write_byte(parent+6,0);
    return 0;
}
static int sigma1_beam_cleared(uint32_t actor,unsigned seat) {
    if(!split_views || !split_valid || !enrolled || failed)return 0;
    uint32_t parent;
    if(actor>=0x80096D98u && actor<0x80098118u && !((actor-0x80096D98u)%0x9Cu) &&
       psx_mod_read_byte(actor+1)==0x20)
        parent=psx_mod_read_word(actor+0x7C);
    else if(actor>=0x800D1F40u && actor<0x800D4F40u && !((actor-0x800D1F40u)%0x60u) &&
            psx_mod_read_byte(actor+1)==0x29)
        parent=psx_mod_read_word(actor+0x50);
    else return 0;
    if(!sigma1_fixed_emitter(parent))return 0;
    unsigned axis=psx_mod_read_byte(parent+2)<2?14u:10u;
    int at=(int16_t)(both_body_word(seat,axis-2u)>>16);
    /* Keep the original shared beam, contacts and impact animation alive
     * for the trailing hero. Only the pane whose hero has cleared this line
     * hides its beam AND its attached endpoint flare; the emitter remains.
     *96px fits before the final door's physical stop. Backtracking restores
     * the native hazard, with no persistent shutdown state or extra clock. */
    return at-(int16_t)psx_mod_read_half(parent+axis)>=96;
}
static void sigma1_final_floor_wait(CPUState *cpu,uint32_t address) {
    if(cpu->gpr[2] || !sigma1_stage() || psx_mod_local_view_scope() ||
       cpu->gpr[16]!=0x800C94A8u || psx_mod_read_byte(0x800C94A9u)!=6 ||
       psx_mod_read_word(address)!=0x14400008u ||
       psx_mod_read_word(0x800EEEF0u)!=0x0C00A2C6u)return;
    /* The last shaft has a separate one-shot row in its background actor.
     * Use the native pre-copy branch so its completion latch remains unset
     * while a living follower still needs to pass the final beam. */
    for(unsigned seat=0;seat<2;++seat)if(both_living(seat) &&
       (int16_t)(both_body_word(seat,12)>>16)<4576) {
        cpu->gpr[2]=1;return;
    }
}
static int coop_floor_map_copy(CPUState *cpu,uint32_t address) {
    if(address==0x80028B18u && sigma1_stage() && !psx_mod_local_view_scope() &&
       cpu->gpr[31]==0x800EF368u && !cpu->gpr[4] &&
       ((cpu->gpr[7]==0x800F8C50u && cpu->gpr[5]==80 && cpu->gpr[6]==1664) ||
        (cpu->gpr[7]==0x800F8C5Eu && cpu->gpr[5]==1184 && cpu->gpr[6]==1904))) {
        /* Keep the shared native upload/checkpoint. Phase2 above performs
         * this exact copy once both living heroes have actually crossed. */
        cpu->gpr[2]=0;return 1;
    }
    if(!enrolled || failed || psx_mod_local_view_scope() || !dinorex_fire_content() ||
       address!=0x80028B18u || cpu->gpr[31]!=0x800EEB58u ||
       cpu->gpr[4] || cpu->gpr[5]!=32 || cpu->gpr[6]!=2176 ||
       cpu->gpr[7]!=0x800F482Cu ||
       psx_mod_read_word(0x800EEB50u)!=0x0C00A2C6u ||
       psx_mod_read_word(0x800F482Cu)!=0x088000A0u ||
       psx_mod_read_word(0x800F4830u)!=0x00010008u)return 0;
    /* The lower-region command replaces the eight tiles under the final
     * upper head. Keep that row while a living follower still needs it.
     * EEAF0's independent atlas/checkpoint command must proceed for the
     * leader; the original visible area retries this copy on later ticks. */
    for(unsigned seat=0;seat<2;++seat)
        if(mmx5_coop_living(&life,seat) && (both_body_byte(seat,0x5C)&127u) &&
           (int16_t)(both_body_word(seat,12)>>16)<2228) {
            psx_mod_counter_add("mmx5.coop.dinorex-upper-row-waits",1);
            cpu->gpr[2]=0;return 1;
        }
    return 0;
}
static int dinorex_floor_seal(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(!enrolled || failed || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12)!=4 ||
       psx_mod_read_byte(PLAY+13) || psx_mod_read_word(0x800EE970u)!=9u ||
       address!=0x800EEAF0u || psx_mod_read_word(address)!=0x27BDFFE0u ||
       psx_mod_read_word(0x800EEB50u)!=0x0C00A2C6u ||
       actor<0x8009E7A0u || actor>=0x8009F9A0u || (actor-0x8009E7A0u)%0x30u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=1 ||
       psx_mod_read_byte(actor+4)!=1)return 0;
    if(!psx_mod_read_byte(actor+2)) {
        if(area_call || projected || !split_available() ||
           psx_mod_read_word(actor+0x18)!=0x800F4828u ||
           psx_mod_read_half(0x800F4834u)!=2192)return 0;
        unsigned checkpoint=psx_mod_read_byte(PLAY+0x1D);
        int second_needs_entry=life.status[1]==MMX5_COOP_ALIVE && second.body[4]==1 &&
            (int16_t)(split_cameras[1][14]|split_cameras[1][15]<<8)>=2192 &&
            (int16_t)(split_cameras[1][0x28]|split_cameras[1][0x29]<<8)<=2192 &&
            (int16_t)(split_cameras[1][0x2A]|split_cameras[1][0x2B]<<8)<2192 &&
            (int16_t)(le32(second.body+12)>>16)>=2228;
        area_call=1;uint32_t result;
        if(second_needs_entry) {
            /* Admit P2 once. Repeating its already committed2192/2192
             * command prevents the same native leaf from admitting P1. */
            result=later_support_as_second(cpu,address,actor);
            psx_mod_counter_add("mmx5.coop.p2-dinorex-lower-atlas-admissions",1);
        } else {
            uint8_t bounds[4];capture(CAMERA+0x28,bounds,sizeof bounds);
            int advanced=(int16_t)psx_mod_read_half(CAMERA+0x28)>2192 ||
                         (int16_t)psx_mod_read_half(CAMERA+0x2A)>2192;
            result=guest(cpu,address,actor,cpu->gpr[5]);
            /* Union visibility keeps EEAF0 alive for the trailing hero.
             * Its old2192/2192 command must not replace the leading view's
             * later native expansion at CB48 or its deeper fire bounds.
             * The original map/atlas work still runs once. */
            if(advanced)project(CAMERA+0x28,bounds,sizeof bounds);
        }
        if(checkpoint>1 && psx_mod_read_byte(PLAY+0x1D)==1)
            psx_mod_write_byte(PLAY+0x1D,(uint8_t)checkpoint);
        area_call=0;cpu->gpr[2]=result;return 1;
    }
    if(psx_mod_read_byte(actor+2)!=1 ||
       psx_mod_read_word(actor+0x18)!=0x800F4836u ||
       psx_mod_read_half(0x800F483Eu)!=20 || psx_mod_read_half(0x800F4842u)!=2656)return 0;
    /* Subtype1 closes the one floor row at Y2640 after the native body
     * passes Y2692. Closing it for the leader traps a living follower on
     * top. Subtype0's earlier map/atlas transition must remain independent.
     * Wait only for living heroes; then let the original map copy, state
     * advance and retirement run, without setting their success fields. */
    for(unsigned seat=0;seat<2;++seat) {
        if(mmx5_coop_living(&life,seat) && (both_body_byte(seat,0x5C)&127u) &&
           (int16_t)(both_body_word(seat,12)>>16)<2692) {
            psx_mod_counter_add("mmx5.coop.dinorex-floor-seal-waits",1);
            cpu->gpr[2]=0;return 1;
        }
    }
    return 0;
}
static unsigned dinorex_fire_recipient(uint32_t actor,
        const Mmx5CoopViewActor actors[2],unsigned target) {
    if(!dinorex_fire_stage() || !dinorex_fire_actor(actor))return target;
    if(dinorex_sequence.actor!=actor || !psx_mod_read_byte(actor+4))
        memset(&dinorex_sequence,0,sizeof dinorex_sequence);
    if(dinorex_sequence.owner) {
        /* F354C clears phase5 only after the actual burst retracts. Keep
         * its camera through that event; the next idle admission may belong
         * to either living hero. Never restart an active burst at their X.
         * During idle F3C44 still requires that body's X>=1137. Retaining
         * an old living owner behind that threshold stalls the warning and
         * fire even when its partner has reached the lower route. */
        if(psx_mod_read_byte(actor+5))
            return actors[dinorex_sequence.owner-1].active?dinorex_sequence.owner-1:target;
    }
    /* This is a shared forward-moving hazard, not an enemy targeting its
     * nearest victim. F3C44's original admission tests X>=1137. A trailing
     * P1 remains nearer its initial position even after P2 crosses that X.
     * Choose the leading living hero whose native camera reached this route;
     * upper-region heroes must not admit the lower fire through X alone. */
    int found=0;
    for(unsigned seat=0;seat<2;++seat) {
        if(!actors[seat].active ||
           (int16_t)camera_emitter_view(seat,0x28)<2416)continue;
        /* Fresh entry's native192px focus leaves the upper approach at
         * Y2410 while its admitted maxY is already2416. A retry clamps the
         * origin to2416. Use the room command in both cases, so a nearby
         * trailing hero cannot retain the idle controller's ownership. */
        if(!found || actors[seat].x>actors[target].x) {target=seat;found=1;}
    }
    if(found && actors[target].x>=1137) {
        int admitted=dinorex_sequence.owner!=target+1;
        dinorex_sequence.actor=actor;dinorex_sequence.owner=(uint8_t)(target+1);
        if(admitted) {
            memcpy(dinorex_sequence.camera,split_cameras[target],sizeof dinorex_sequence.camera);
            psx_mod_counter_add(target?"mmx5.coop.p2-dinorex-fire-admissions":
                                      "mmx5.coop.p1-dinorex-fire-admissions",1);
        }
    } else memset(&dinorex_sequence,0,sizeof dinorex_sequence);
    return target;
}
static int dinorex_fire_survivor(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(!projected || !world_owner || ai_call || ai_suspended ||
       life.status[0]==MMX5_COOP_ALIVE || life.status[1]!=MMX5_COOP_ALIVE ||
       address!=0x800F36F8u || !dinorex_fire_stage() || !dinorex_fire_actor(actor) ||
       psx_mod_read_word(address)!=0x27BDFFE8u)return 0;
    /* The sole-P2 world bypasses ai_update. Preserve the same burst camera
     * there too: switching the foreground reference halfway through native
     * F354C can retract the sheet before F3E7C reaches its wall command. */
    Mmx5CoopViewActor actors[2];split_actors(actors);
    for(unsigned seat=0;seat<2;++seat)actors[seat].active=life.status[seat]==MMX5_COOP_ALIVE;
    (void)dinorex_fire_recipient(actor,actors,1);
    if(!dinorex_sequence.owner)return 0;
    unsigned owner=dinorex_sequence.owner-1,camera_owner=split_owner;
    uint8_t canonical[0xFC],view[0xFC];capture(CAMERA,canonical,sizeof canonical);
    memcpy(view,actors[owner].active?split_cameras[owner]:dinorex_sequence.camera,sizeof view);
    memcpy(view+0x54,canonical+0x54,0x54);
    project(CAMERA,view,sizeof view);split_owner=owner+1;
    ai_suspended=1;uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);ai_suspended=0;
    capture(CAMERA,dinorex_sequence.camera,sizeof canonical);
    if(actors[owner].active)capture(CAMERA,split_cameras[owner],sizeof canonical);
    capture(CAMERA+0x54,canonical+0x54,0x54);
    project(CAMERA,canonical,sizeof canonical);split_owner=camera_owner;
    cpu->gpr[2]=result;return 1;
}
static int dinorex_fire_wall_burst(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(area_call || !dinorex_fire_stage() || !dinorex_fire_actor(actor) ||
       address!=0x800F3E7Cu || psx_mod_read_word(address)!=0x27BDFFD0u)return 0;
    /* The shared wave is visible in each lower-room view. Its original
     * wall event must likewise see the leading living view, even while
     * the sheet retains a more distant admitting camera after a fatal. */
    unsigned target=2;int leading=-32768;
    for(unsigned seat=0;seat<2;++seat)if(both_living(seat) &&
       (int16_t)camera_emitter_view(seat,0x2A)>=2656) {
        int x=(int16_t)camera_emitter_view(seat,10);
        if(x>leading) {leading=x;target=seat;}
    }
    if(target>1)return 0;
    uint16_t x=psx_mod_read_half(CAMERA+10);
    psx_mod_write_half(CAMERA+10,(uint16_t)leading);
    area_call=1;uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);area_call=0;
    psx_mod_write_half(CAMERA+10,x);
    cpu->gpr[2]=result;return 1;
}
static int dinorex_dragon_actor(uint32_t actor) {
    return split_available() && psx_mod_read_byte(PLAY)==10 &&
        psx_mod_read_byte(PLAY+12)==4 && psx_mod_read_byte(PLAY+13)==1 &&
        psx_mod_read_word(0x800EE970u)==10u &&
        psx_mod_read_word(0x800FE680u)==0x800F3780u &&
        psx_mod_read_word(0x800FE684u)==0x800F3FC8u &&
        psx_mod_read_word(0x800F4240u)==0xA602000Au &&
        psx_mod_read_word(0x800F4260u)==0xA603000Au &&
        actor>=0x80092090u && actor<0x80093DD0u &&
        (actor-0x80092090u)%0x9Cu==0 && psx_mod_read_byte(actor) &&
        psx_mod_read_byte(actor+1)==0x22 && !psx_mod_read_byte(actor+2);
}
static unsigned dinorex_dragon_recipient(uint32_t actor,
        const Mmx5CoopViewActor actors[2],unsigned target) {
    if(!dinorex_dragon_actor(actor))return target;
    if(dinorex_sequence.actor!=actor || !psx_mod_read_byte(actor+4))
        memset(&dinorex_sequence,0,sizeof dinorex_sequence);
    if(!actors[target].active)return target;
    /* Placement admission supplies the initial nearby body. Keep that
     * encounter's camera while its owner lives: a hero at a later exit
     * must not relocate the chaser or consume its initial room lock. */
    if(!dinorex_sequence.owner)dinorex_sequence.owner=(uint8_t)(target+1);
    unsigned camera=dinorex_sequence.owner-1;
    if(actors[camera].active)target=camera;
    else camera=target;
    /* Preserve the native sequence view on death until a survivor reaches
     * it; its movement, attacks, camera commands and lifetime still run once. */
    if(dinorex_sequence.owner &&
       !actors[dinorex_sequence.owner-1].active &&
       (int16_t)(split_cameras[camera][10]|split_cameras[camera][11]<<8)<
       (int16_t)(dinorex_sequence.camera[10]|dinorex_sequence.camera[11]<<8)) {
        psx_mod_counter_add("mmx5.coop.dinorex-dragon-retained-camera-ticks",1);
    } else {
        dinorex_sequence.owner=(uint8_t)(camera+1);
        memcpy(dinorex_sequence.camera,split_cameras[camera],sizeof dinorex_sequence.camera);
    }
    dinorex_sequence.actor=actor;
    return target;
}
static int dinorex_dragon_survivor(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(!projected || !world_owner || ai_call || ai_suspended ||
       life.status[0]==MMX5_COOP_ALIVE || life.status[1]!=MMX5_COOP_ALIVE ||
       address!=0x800F43E8u || !dinorex_dragon_actor(actor))return 0;
    /* The sole-P2 world already supplies the correct body and deliberately
     * bypasses ai_pool_tick. Keep only the chaser's sequence camera here;
     * projecting P2 again would overwrite the dead P1 context. */
    Mmx5CoopViewActor actors[2];split_actors(actors);
    for(unsigned seat=0;seat<2;++seat)actors[seat].active=life.status[seat]==MMX5_COOP_ALIVE;
    (void)dinorex_dragon_recipient(actor,actors,1);
    if(!dinorex_sequence.owner)return 0;
    unsigned owner=dinorex_sequence.owner-1;
    uint8_t canonical[0xFC];capture(CAMERA,canonical,sizeof canonical);
    project(CAMERA,actors[owner].active?split_cameras[owner]:dinorex_sequence.camera,sizeof canonical);
    ai_suspended=1;uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);ai_suspended=0;
    capture(CAMERA,dinorex_sequence.camera,sizeof canonical);
    if(actors[owner].active)capture(CAMERA,split_cameras[owner],sizeof canonical);
    project(CAMERA,canonical,sizeof canonical);
    cpu->gpr[2]=result;return 1;
}
static void grizzly_cannon_camera(uint32_t actor,const uint8_t before[8],uint8_t *canonical) {
    if(!split_available() || !grizzly_stage() || psx_mod_read_byte(actor+1)!=0x10)return;
    /* F58BC arms a shared train stop once, and releases it on real defeat.
     * The selected body's native AI still runs once. Publish only bounds
     * actually changed by that command; private XY/history remain private.
     * P2's AI scope restores its saved canonical camera afterward. Merge the
     * same shared command there so that restore cannot undo P1's stop/release. */
    int changed=0;
    for(unsigned i=0;i<8;i+=2) {
        uint8_t lo=psx_mod_read_byte(CAMERA+0x24+i),hi=psx_mod_read_byte(CAMERA+0x25+i);
        if(lo==before[i] && hi==before[i+1])continue;
        for(unsigned seat=0;seat<2;++seat) {
            split_cameras[seat][0x24+i]=lo;split_cameras[seat][0x25+i]=hi;
            split_cameras[seat][0x47]=psx_mod_read_byte(CAMERA+0x47);
        }
        if(canonical) {
            canonical[0x24+i]=lo;canonical[0x25+i]=hi;
            canonical[0x47]=psx_mod_read_byte(CAMERA+0x47);
        }
        changed=1;
    }
    if(changed)psx_mod_counter_add("mmx5.coop.shared-train-camera-commands",1);
}
static int intro_boss_camera_actor(uint32_t actor,uint32_t function) {
    /* The head's F7EF0 callback writes a shared background atlas origin.
     * Its body may target P2, but restoring a borrowed camera discards that
     * origin while the native head/attack sprites continue moving. */
    return function==0x800F7EF0u && psx_mod_read_byte(PLAY)==10 &&
        !psx_mod_read_byte(PLAY+12) && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==4 &&
        psx_mod_read_word(PLAY+0x20)==actor &&
        psx_mod_read_word(0x800F7F50u)==0xA0600057u &&
        psx_mod_read_word(0x800F9B34u)==0xA462005Eu &&
        psx_mod_read_word(0x800F9B48u)==0xA4620062u &&
        psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==0x31 &&
        !psx_mod_read_byte(actor+2);
}
static void ai_update(CPUState *cpu,uint32_t address) {
    (void)address;uint32_t actor=ai_pending,function=ai_function;
    ai_pending=ai_function=0;if(!actor || !function || ai_call || !ai_pool_call)return;
    if(both_door_pending_boss(actor)) {
        psx_mod_counter_add("mmx5.coop.both-door-boss-admission-waits",1);
        cpu->gpr[2]=0;return;
    }
    if(duel_hold_opponent(cpu,actor)) {cpu->gpr[2]=0;return;}
    psx_mod_write_byte(PLAYER+0xCC,ai_first_freeze);
    Mmx5CoopViewActor actors[2];split_actors(actors);
    for(unsigned seat=0;seat<2;++seat) {
        /* Incoming/outgoing actors retain their camera and lifetime region,
         * but must not own native AI or a newly acquired script. A refight
         * boss can otherwise select P2's incoming beam above the room and
         * freeze that same arrival by making P2 the shared scene owner. */
        actors[seat].active=life.status[seat]==MMX5_COOP_ALIVE;
    }
    int origin_x=(int16_t)psx_mod_read_half(actor+10);
    int origin_y=(int16_t)psx_mod_read_half(actor+14);
    int parent_view=(psx_mod_read_byte(PLAY+12)==12 &&
        !psx_mod_read_byte(PLAY+13) && psx_mod_read_word(0x800EE970u)==22u &&
        psx_mod_read_word(0x800F4B94u)==0x0C00B24Eu &&
        psx_mod_read_word(0x800F4BD8u)==0xACA20080u) ||
        (psx_mod_read_byte(PLAY+12)==6 && psx_mod_read_byte(PLAY+13)==1 &&
         psx_mod_read_word(0x800EE970u)==13u &&
         psx_mod_read_word(0x800EF5D8u)==0x0C00B24Eu &&
         psx_mod_read_word(0x800EF61Cu)==0xACA20080u);
    if(!origin_x && !origin_y && parent_view &&
       psx_mod_read_byte(actor+1)==0x1Bu && psx_mod_read_byte(actor+2)>=1 &&
       psx_mod_read_byte(actor+2)<=3 && !psx_mod_read_word(actor+0x10)) {
        /* Native F4B5C and Glow F5A0 allocate children before they have XY.
         * Their +80 points into the producer, whose encounter view must own
         * that first placement. Nearest-to-(0,0) instead selects a hero at
         * the stage entrance and materializes the group in the wrong pane.
         * Once native placement establishes XY, ordinary targeting resumes. */
        uint32_t parent=psx_mod_read_word(actor+0x80)-0x80u;
        if(parent>=0x80092090u && parent<0x80093DD0u &&
           !((parent-0x80092090u)%0x9Cu) && psx_mod_read_byte(parent) &&
           psx_mod_read_byte(parent+1)==0x1Bu && !psx_mod_read_byte(parent+2)) {
            origin_x=(int16_t)psx_mod_read_half(parent+10);
            origin_y=(int16_t)psx_mod_read_half(parent+14);
        }
    }
    unsigned target=scene_owner?scene_owner-1:mmx5_coop_view_nearest(actors,origin_x,origin_y,0);
    if(!scene_owner) {
        target=field_recipient(actor,target);
        target=grab_recipient(cpu,actor,target);
        target=puffer_recipient(actor,target);
        target=dinorex_fire_recipient(actor,actors,target);
        target=dinorex_dragon_recipient(actor,actors,target);
    }
    /* The story opponent's native introduction addresses the hero who
     * entered its door. A nearby companion must not steal its stand/release
     * commands after both players have crossed that door together. */
    if(duel_scope() && actor==story_duel.opponent)target=story_duel.entrant;
    ai_call=1;ai_owner=target;uint32_t result=0;
    if(actors[target].active) {
        uint8_t canonical_camera[0xFC];unsigned camera_owner=split_owner;
        unsigned camera_seat=target;
        int retained_fire=dinorex_fire_stage() && dinorex_fire_actor(actor) &&
            dinorex_sequence.actor==actor && dinorex_sequence.owner;
        int dragon_camera=dinorex_dragon_actor(actor);
        int retained_dragon=dragon_camera && dinorex_sequence.actor==actor && dinorex_sequence.owner;
        if(retained_fire || retained_dragon)camera_seat=dinorex_sequence.owner-1;
        int fire_camera=dinorex_fire_stage() && dinorex_fire_actor(actor);
        /* Whale rooms have one authored camera. Class1E's root writes the
         * hull layer origin after moving; restoring P1's pre-AI camera when
         * P2 is nearest discards that write while child sprites keep moving.
         * Keep nearest-player targeting, but publish the native camera write. */
        int private_camera=!fire_camera && !dragon_camera && target && split_available() && !scene_owner &&
            !whale_authored_camera() && !pegasus_platform_camera() &&
            !intro_boss_camera_actor(actor,function);
        if(fire_camera || dragon_camera || private_camera) {
            /* Ordinary AI borrows its victim's view. Persistent stage
             * sequences retain their admitted camera independently. */
            capture(CAMERA,canonical_camera,sizeof canonical_camera);
            const uint8_t *view=(retained_fire || retained_dragon) && !actors[camera_seat].active?
                dinorex_sequence.camera:split_cameras[camera_seat];
            uint8_t fire_view[0xFC];
            if(fire_camera) {
                /* Only the foreground view belongs to the admitting hero.
                 * The original world draw wraps/refreshes the fire layer;
                 * a saved private layer predates those committed writes. */
                memcpy(fire_view,view,sizeof fire_view);
                memcpy(fire_view+0x54,canonical_camera+0x54,0x54);
                view=fire_view;
            }
            /* P1 already owns the current native camera. Its retained view
             * may predate this frame's layer initialization/area command. */
            if(dragon_camera || camera_seat || (retained_fire && !actors[camera_seat].active))
                project(CAMERA,view,sizeof canonical_camera);
            split_owner=camera_seat+1;
        }
        /* Native grabs/status changes can call player-part helpers too.
         * Select the matching body AND character resources, preserving the
         * whole-stage palette and campaign. Nested contacts restore the
         * normal two-player ownership boundary before resuming this scope. */
        if(target) {
            uint8_t campaign=psx_mod_read_byte(PLAY+0x44);enter_second();world_owner=1;
            psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
        }
        if(private_camera || fire_camera)ai_camera_restore=canonical_camera;
        if(psx_mod_read_byte(PLAYER+0xCC)) {
            if(psx_mod_read_byte(actor+3))result=guest(cpu,0x8002DAA0,actor,0);
        } else {
            uint8_t camera_bounds[8];capture(CAMERA+0x24,camera_bounds,sizeof camera_bounds);
            int tracked=balance_begin(actor,function);
            result=guest(cpu,function,actor,function);
            if(tracked)balance_end();
            grizzly_cannon_camera(actor,camera_bounds,private_camera?canonical_camera:NULL);
            if(dragon_camera && !camera_seat && actors[0].active) {
                /* F3888's actual left-lock command must survive the world
                 * epilogue. Merge only bounds changed by this native call;
                 * other bounds may already contain a newer area command. */
                for(unsigned i=0;i<sizeof camera_bounds;i+=2) {
                    uint16_t value=psx_mod_read_half(CAMERA+0x24+i);
                    uint16_t before=(uint16_t)(camera_bounds[i]|camera_bounds[i+1]<<8);
                    if(value!=before)both_half(canonical_camera,0x24+i,value);
                }
            }
            psx_mod_counter_add(target?"mmx5.coop.p2-native-ai":"mmx5.coop.p1-native-ai",1);
        }
        ai_camera_restore=NULL;
        if(target) {leave_second_context(1);world_owner=0;}
        if(fire_camera || dragon_camera) {
            if(retained_fire || retained_dragon)capture(CAMERA,dinorex_sequence.camera,sizeof canonical_camera);
            if(actors[camera_seat].active)
                capture(CAMERA,split_cameras[camera_seat],sizeof canonical_camera);
            if(fire_camera) {
                /* Native272B4 commits the shared fire tile wrapping once,
                 * including after admitting-hero death. Private draws discard
                 * those writes, so publish its one world layer here. */
                capture(CAMERA+0x54,canonical_camera+0x54,0x54);
            }
            if(dragon_camera || camera_seat || !actors[camera_seat].active)
                project(CAMERA,canonical_camera,sizeof canonical_camera);
            split_owner=camera_owner;
        } else if(private_camera) {
            /* Nearest-player AI also reads CAMERA for native admissions,
             * emitters and scripted bounds. Supplying P2's body with P1's
             * camera leaves Glow's stair encounter/viruses waiting for P1.
             * Retain the selected view's real commands, and keep a newly
             * acquired P2 scene camera authoritative for this world pass. */
            capture(CAMERA,split_cameras[target],sizeof canonical_camera);
            if(scene_owner!=target+1 && !pegasus_platform_camera()) {
                project(CAMERA,canonical_camera,sizeof canonical_camera);
                split_owner=camera_owner;
            }
        }
    }
    ai_first_freeze=psx_mod_read_byte(PLAYER+0xCC);psx_mod_write_byte(PLAYER+0xCC,0);
    ai_call=ai_owner=0;cpu->gpr[2]=result;
}
static int rose_capture_tick(CPUState *cpu,uint32_t address) {
    if(ai_pool_call || ai_call || !enrolled || failed || split_available() ||
       psx_mod_read_byte(PLAY)!=10)return 0;
    unsigned family=rose_capture_actor(cpu->gpr[4]);
    uint32_t function=family==1?0x800F2448u:
        psx_mod_read_byte(PLAY+12)==12?0x800FE0C8u:0x800F6EFCu;
    if(!family || address!=function)return 0;
    if(projected)return world_owner?canonical_world_call(cpu,address,rose_capture_tick):0;
    /* Unified cameras normally keep the original P1 AI pass. A capturing
     * actor must instead execute its contact AND immediate CA/pose checks
     * against one body, then retain that recipient through native release.
     * Reuse the existing per-actor dispatcher only for these two programs;
     * ordinary unified AI, the world clock and its freeze state stay native. */
    uint8_t freeze=psx_mod_read_byte(PLAYER+0xCC);
    ai_pool_call=1;ai_pool=address;ai_first_freeze=freeze;
    ai_pending=cpu->gpr[4];ai_function=address;
    ai_update(cpu,AI_DISPATCH);
    psx_mod_write_byte(PLAYER+0xCC,freeze);
    ai_pool_call=0;ai_pool=0;ai_first_freeze=0;
    psx_mod_counter_add("mmx5.coop.rose-unified-capture-ticks",1);
    return 1;
}
static int ai_pool_tick(CPUState *cpu,uint32_t address) {
    if(ai_pool_call || projected || car_context() ||
       (!split_available() && !duel_scope() && !both_door_active()))return 0;
    ai_pool_call=1;ai_pool=address;ai_first_freeze=psx_mod_read_byte(PLAYER+0xCC);
    psx_mod_write_byte(PLAYER+0xCC,0);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    psx_mod_write_byte(PLAYER+0xCC,ai_first_freeze);ai_first_freeze=0;
    ai_pool_call=0;ai_pool=0;cpu->gpr[2]=result;return 1;
}
static unsigned split_view_seat(unsigned seat) {
    if(seat>1)seat=0;
    int dead0=life.status[0]==MMX5_COOP_DYING || life.status[0]==MMX5_COOP_FALLEN;
    int dead1=life.status[1]==MMX5_COOP_DYING || life.status[1]==MMX5_COOP_FALLEN;
    if(dead0 && dead1)return seat;
    if(scene_owner && !duel_fill_pair_present())return scene_owner-1;
    /* Native fatal entry still has a pose and an effect sequence. Keep that
     * seat's view until it finishes; early spectator fallback hides a distant
     * partner's entire death while it is still running. */
    return mmx5_coop_spectator_seat(&life,seat);
}
static void split_scenery_origins(CPUState *cpu) {
    /* BIN0's class36 floor controller precomputes screen X once from the
     * canonical camera. Rebuild its origin and native visibility for this
     * private draw; do not rerun its animation or placement controller. */
    if(!psx_mod_local_view_scope() || !split_views || !enrolled || failed ||
       psx_mod_read_byte(PLAY+12) || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=4u ||
       psx_mod_read_word(0x800F1208u)!=0x8444A202u ||
       psx_mod_read_word(0x800F123Cu)!=0xA602000Au)return;
    int camera=(int16_t)psx_mod_read_half(CAMERA+10);
    for(uint32_t actor=0x800D1F40u;actor<0x800D4F40u;actor+=0x60u) {
        if((psx_mod_read_byte(actor)&0x60u)!=0x40u || psx_mod_read_byte(actor+1)!=0x36u ||
           (int8_t)psx_mod_read_byte(actor+0x14)>=0)continue;
        int x=psx_mod_read_half(actor+0x54)-(5*camera)/4;
        psx_mod_write_half(actor+10,(uint16_t)x);
        cpu->gpr[6]=0x60u;guest(cpu,0x8002D9ECu,actor,0x60u);
        psx_mod_counter_add("mmx5.coop.local-scenery-origin",1);
    }
}
static void intro_scenery_checkpoint(CPUState *cpu,uint32_t address) {
    (void)address;uint32_t actor=cpu->gpr[16];
    if(!split_available() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY+12) || psx_mod_read_byte(PLAY+13) ||
       !cpu->gpr[2] || psx_mod_read_word(0x800EE970u)!=4u ||
       psx_mod_read_word(0x800F1200u)!=0x14400011u ||
       psx_mod_read_word(0x800F1208u)!=0x8444A202u ||
       actor<0x800D1F40u || actor>=0x800D4F40u || (actor-0x800D1F40u)%0x60u ||
       psx_mod_read_byte(actor+1)!=0x36u)return;
    /* Native F11BC retires these original foreground props after the
     * singleton checkpoint advances. A distant old-region view still needs
     * them. Retain the native actors/animation until the last visible hero
     * leaves; private draws already recompute their origin and visibility.
     * Do not roll back the checkpoint or synthesize replacement objects. */
    for(unsigned seat=0;seat<2;++seat)if(
       (mmx5_coop_living(&life,seat) || life.status[seat]==MMX5_COOP_DYING) &&
       (int16_t)(split_cameras[seat][10]|split_cameras[seat][11]<<8)<2568) {
        /* Change this actor's retirement predicate, not the shared native
         * checkpoint. Its original origin/cull path still runs this frame. */
        cpu->gpr[2]=0;
        psx_mod_counter_add("mmx5.coop.intro-foreground-retirement-waits",1);
        return;
    }
}
static void shared_atlas_view(uint8_t *view,const uint8_t *canonical) {
    /* The final-stage overlays select shared layer2 atlas frames and mark their
     * native refresh byte. Mode5's
     * ordinary camera update does not regenerate these origins for P2.
     * Publish only the producer's fields before that private camera update;
     * keep its histories, bounds and the one original animation clock. */
    unsigned stage=psx_mod_read_byte(PLAY+12),overlay=psx_mod_read_word(0x800EE970u);
    int sigma1=stage==16 && overlay==0x1Eu &&
        psx_mod_read_word(0x800EF144u)==0xA4A400B2u &&
        psx_mod_read_word(0x800EF160u)==0xA0A200F4u &&
        psx_mod_read_word(0x800EF164u)==0xA4A400B6u;
    int sigma2=stage==17 && overlay==0x1Fu &&
        psx_mod_read_word(0x800EF7E0u)==0xA4A400B2u &&
        psx_mod_read_word(0x800EF7FCu)==0xA0A200F4u &&
        psx_mod_read_word(0x800EF800u)==0xA4A400B6u;
    int sigma4=stage==12 && overlay==0x16u &&
        psx_mod_read_word(0x800EF0B4u)==0xA4A400B2u &&
        psx_mod_read_word(0x800EF0D0u)==0xA0A200F4u &&
        psx_mod_read_word(0x800EF0D4u)==0xA4A400B6u;
    int sigma3=stage==18 && overlay==0x20u &&
        psx_mod_read_word(0x800EF2F4u)==0xA4A400B2u &&
        psx_mod_read_word(0x800EF310u)==0xA0A200F4u &&
        psx_mod_read_word(0x800EF314u)==0xA4A400B6u;
    if(psx_mod_read_byte(PLAY+13) || canonical[0xAC]!=5 ||
       (!sigma1 && !sigma2 && !sigma3 && !sigma4))return;
    int changed=memcmp(view+0xB2,canonical+0xB2,2) ||
                memcmp(view+0xB6,canonical+0xB6,2);
    memcpy(view+0xB2,canonical+0xB2,2);
    memcpy(view+0xB6,canonical+0xB6,2);
    view[0xF4]=canonical[0xF4];
    if(changed)psx_mod_counter_add("mmx5.coop.shared-atlas-published",1);
}
static int rose_stage_program(void) {
    return psx_mod_read_byte(PLAY+12)==7 && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==14 &&
        psx_mod_read_word(0x800EECDCu)==0x27BDFFE0u &&
        psx_mod_read_word(0x800EEDD0u)==0xA44300E8u &&
        psx_mod_read_word(0x800EEDD4u)==0xA04400F4u;
}
static void rose_private_layer_region(void) {
    /* BIN10's checkpoint selects a background region for its one camera.
     * Split views can still occupy earlier rooms after that shared progress
     * advances. Select each view's region from its native room/body instead;
     * EEDD8's map mutation and actor lifecycle remain shared. */
    if(!rose_stage_program())return;
    int x=(int16_t)psx_mod_read_half(PLAYER+10);
    int y=(int16_t)psx_mod_read_half(PLAYER+14);
    /* Area0 admits the vine shaft at X1744 and sets its native minimum1600.
     * A teammate still before that room retains the initial atlas region.
     * The upper/lower and right-shaft divisions are EECDC's own thresholds. */
    unsigned offset=y<769?800:x>=3889?1900:
        x<1744 && (int16_t)psx_mod_read_half(CAMERA+0x26)<1600?0:1200;
    if(psx_mod_read_half(CAMERA+0xE8)!=offset) {
        psx_mod_write_half(CAMERA+0xE8,(uint16_t)offset);
        psx_mod_write_byte(CAMERA+0xF4,1);
    }
}
static int rose_floor_recipient(void) {
    int recipient=-1;
    for(unsigned seat=0;seat<2;++seat)if(both_living(seat)) {
        int x=(int16_t)(both_body_word(seat,8)>>16);
        int y=(int16_t)(both_body_word(seat,12)>>16);
        const uint8_t *view=split_cameras[seat];
        int lower_room=(int16_t)(view[0x26]|view[0x27]<<8)>=1920 &&
                       (int16_t)(view[0x2A]|view[0x2B]<<8)==960;
        /* The stage begins at the same height as the later lower route.
         * Its early room is still before this drop; height alone cannot
         * establish that every living hero is clear of the sealing row. */
        if(y<992 || (x<4160 && !lower_room))return -1;
        if(x>=4160 && x<4560 && y<1152 &&
           (int16_t)(view[14]|view[15]<<8)==960)recipient=(int)seat;
    }
    return recipient;
}
static void rose_floor_predicate(CPUState *cpu,uint32_t address) {
    if(!split_available() || (projected && !world_owner) ||
       !rose_stage_program() || psx_mod_read_byte(PLAY+0x2F) ||
       psx_mod_read_word(0x800EEE4Cu)!=0x0C00A2C6u)return;
    int recipient=rose_floor_recipient();
    if(address==0x800EEE0Cu) {
        /* Preserve the native map copy, sound and one-shot state. Only its
         * spatial admission waits for the other living body, and either
         * hero can supply the last arrival after the first has moved on. */
        if(cpu->gpr[2] && recipient<0)
            psx_mod_counter_add("mmx5.coop.rose-floor-seal-waits",1);
        cpu->gpr[2]=(uint32_t)(recipient>=0);
    } else if(recipient>=0 && address==0x800EEE1Cu) {
        cpu->gpr[3]=(uint32_t)(int32_t)(int16_t)(both_body_word((unsigned)recipient,12)>>16);
    } else if(recipient>=0 && address==0x800EEE34u) {
        const uint8_t *view=split_cameras[recipient];
        cpu->gpr[4]=(uint32_t)(int32_t)(int16_t)(view[14]|view[15]<<8);
    }
}
static void refight_hub_seal(CPUState *cpu,uint32_t address) {
    if(!split_available() || (projected && !world_owner) ||
       psx_mod_read_byte(PLAY+12)!=12 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=22u ||
       address!=0x800EEDD4u || psx_mod_read_word(address)!=0x14400016u ||
       psx_mod_read_word(0x800EEE20u)!=0x0C00A2C6u)return;
    /* The original one-shot map replacement seals the entry floor. Keep
     * its mutation and latch together, waiting until every living hero has
     * actually dropped into the hub instead of sealing behind the leader. */
    for(unsigned seat=0;seat<2;++seat)if(both_living(seat)) {
        int x=(int16_t)(both_body_word(seat,8)>>16);
        int y=(int16_t)(both_body_word(seat,12)>>16);
        if(x<2976 || y<1472) {
            cpu->gpr[2]=1;
            psx_mod_counter_add("mmx5.coop.refight-hub-seal-waits",1);
            return;
        }
    }
}
static int glow_stair_program(void) {
    return psx_mod_read_byte(PLAY+12)==6 && psx_mod_read_byte(PLAY+13)==1 &&
        psx_mod_read_word(0x800EE970u)==13 &&
        psx_mod_read_word(0x800FA3C4u)==0x800F72C0u &&
        psx_mod_read_word(0x800F7194u)==0x00803021u &&
        psx_mod_read_word(0x800F72B4u)==0xA4620062u;
}
static void glow_stair_private(CPUState *cpu,uint32_t actor) {
    uint8_t canonical[4];capture(actor+0x14,canonical,sizeof canonical);
    if(!glow_stair.valid || glow_stair.actor!=actor) {
        glow_stair.actor=actor;memcpy(glow_stair.pose,canonical,sizeof canonical);glow_stair.valid=1;
    }
    project(actor+0x14,glow_stair.pose,sizeof canonical);
    unsigned old_call=area_call;area_call=1;
    guest(cpu,0x800F7194u,actor,0);area_call=old_call;
    capture(actor+0x14,glow_stair.pose,sizeof canonical);
    project(actor+0x14,canonical,sizeof canonical);
    psx_mod_counter_add("mmx5.coop.p2-stair-atlas-ticks",1);
}
static int glow_stair_native(CPUState *cpu,uint32_t address) {
    (void)address;
    if(area_call || !split_available() || !glow_stair_program())return 0;
    /* An old-region view can retain the producer after P1 has left. Its
     * original leaf must then stop writing P1's later-region layer. */
    if((int16_t)psx_mod_read_half(PLAYER+10)>=2240) {cpu->gpr[2]=0;return 1;}
    if(projected && world_owner) {
        glow_stair_private(cpu,cpu->gpr[4]);cpu->gpr[2]=0;return 1;
    }
    return 0;
}
static void glow_stair_retirement(CPUState *cpu,uint32_t address) {
    (void)address;uint32_t actor=cpu->gpr[4];
    if(!split_available() || projected || !glow_stair_program() ||
       actor<0x8009E7A0u || actor>=0x8009F9A0u || (actor-0x8009E7A0u)%0x30u ||
       psx_mod_read_byte(actor+1)!=12 || !mmx5_coop_living(&life,1) ||
       (int16_t)(le32(second.body+8)>>16)>=2240)return;
    cpu->gpr[2]=1; /* Original retirement waits for the last stair resident. */
}
static void glow_stair_view(CPUState *cpu) {
    if(!glow_stair_program() || psx_mod_read_byte(PLAY+0x26) ||
       (int16_t)(le32(second.body+8)>>16)>=2240 || psx_mod_read_byte(CAMERA+0x58)!=5)return;
    for(uint32_t actor=0x8009E7A0u;actor<0x8009F9A0u;actor+=0x30u)
        if(psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==12 &&
           psx_mod_read_byte(actor+4)==1) {
            /* Only the original camera/atlas leaf runs for the extra view.
             * Its four spatial bytes are private; no shared actor lifecycle,
             * world clock, enemy or encounter program advances twice. */
            glow_stair_private(cpu,actor);break;
        }
}
static void shared_scroll_view(uint8_t *view,const uint8_t *canonical) {
    /* Pegasus's reserved class3 cloud program advances one world clock at
     * EEACC, including its original 1024-pixel wrap and refresh command.
     * Mode5's private layer callback consumes that origin without ticking it.
     * Publish the completed cloud origin; foreground/parallax follow stays
     * private, and no second controller or clock update is run. */
    if(psx_mod_read_byte(PLAY+12)==8 && !psx_mod_read_byte(PLAY+13) &&
       psx_mod_read_word(0x800EE970u)==0x0Fu && canonical[0xAC]==5 && view[0xAC]==5 &&
       psx_mod_read_word(0x800EEAE8u)==0xACC200B0u &&
       psx_mod_read_word(0x800EEAFCu)==0xACC300B4u &&
       psx_mod_read_word(0x800EEB30u)==0xA4C200B6u) {
        for(uint32_t actor=0x800C94A8u;actor<0x800C9568u;actor+=0x30u)
            if(psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==3 &&
               psx_mod_read_byte(actor+4)==1 &&
               psx_mod_read_word(actor+0x10)==0x800F8F34u) {
                memcpy(view+0xB0,canonical+0xB0,8);view[0xF4]=canonical[0xF4];
                psx_mod_counter_add("mmx5.coop.shared-cloud-scroll-published",1);
                return;
            }
    }
    /* BIN1's reserved train controller advances these spatial origins once
     * per world update, independently of the foreground camera. Mode5's
     * ordinary layer callbacks do not advance that external clock for P2.
     * Publish the native final (already wrapped) X origins and refresh flags;
     * retain each view's Y, histories and bounds. F7's camera-delta branch
     * and other layer modes remain outside this shared-clock path. */
    if(psx_mod_read_byte(PLAY+12)!=1 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=5u || psx_mod_read_byte(PLAYER+0xF7) ||
       canonical[0x58]!=5 || canonical[0xAC]!=5 ||
       view[0x58]!=5 || view[0xAC]!=5 ||
       psx_mod_read_word(0x800EF158u)!=0x27BDFFE8u ||
       psx_mod_read_word(0x800EF1F4u)!=0xACA2005Cu ||
       psx_mod_read_word(0x800EF264u)!=0xA4A2005Eu ||
       psx_mod_read_word(0x800EF398u)!=0xAC4400B0u)return;
    unsigned active=0;
    for(uint32_t actor=0x800C94A8u;actor<0x800C9568u;actor+=0x30u)
        if(psx_mod_read_byte(actor) && !psx_mod_read_byte(actor+1) &&
           psx_mod_read_byte(actor+4)==1 && psx_mod_read_byte(actor+5)<7 &&
           psx_mod_read_word(actor+0x10)==0x800FBFA4u)active=1;
    if(!active)return;
    memcpy(view+0x5C,canonical+0x5C,4);
    memcpy(view+0xB0,canonical+0xB0,4);
    view[0xA0]=canonical[0xA0];view[0xF4]=canonical[0xF4];
    psx_mod_counter_add("mmx5.coop.shared-scroll-published",1);
}
static int intro_layer_region_ready(void) {
    return split_available() && !psx_mod_read_byte(PLAY+12) && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==4 && psx_mod_read_word(0x800E92F8u)==2 &&
        psx_mod_read_word(0x8007C760u)==0xFFFF0A08u &&
        psx_mod_read_word(0x800EEC00u)==0xA0430058u &&
        psx_mod_read_word(0x800EEC04u)==0xA4400096u &&
        psx_mod_read_word(0x800EEC08u)==0xA4400094u;
}
static void intro_private_layer_region(void) {
    /* The authored atlas area begins at camera X2568. Its one shared loader
     * may finish while the other view is still in the parallax city. Apply
     * the admitted native layer command when this camera reaches that area,
     * before its original secondary-layer update, not to both views at once. */
    if(psx_mod_local_view_scope() || !intro_layer_region_ready() ||
       psx_mod_read_byte(CAMERA+0x58)!=6 || (int16_t)psx_mod_read_half(CAMERA+10)<2568)return;
    psx_mod_write_byte(CAMERA+0x58,2);
    psx_mod_write_half(CAMERA+0x94,0);psx_mod_write_half(CAMERA+0x96,0);
}
static void shared_intro_layer_command(CPUState *cpu,uint32_t address) {
    (void)address;
    /* BIN0 EEBE0 changes layer1 from parallax to foreground coordinates
     * after the building's shared atlas request. Publish that original
     * command only to views already in its region; the primary-camera
     * callbacks admit a trailing view when it catches up. Mode2 is also
     * required for the later statue's native visibility calculation. */
    if(!split_available() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY+12) || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=4 || cpu->gpr[2]!=CAMERA ||
       psx_mod_read_word(0x800EEBE0u)!=0x00802821u ||
       psx_mod_read_word(0x800EEC00u)!=0xA0430058u ||
       psx_mod_read_word(0x800EEC04u)!=0xA4400096u ||
       psx_mod_read_word(0x800EEC08u)!=0xA4400094u ||
       psx_mod_read_word(0x800EEC0Cu)!=0x90A30005u)return;
    uint8_t offsets[4];capture(CAMERA+0x94,offsets,sizeof offsets);
    for(unsigned seat=0;seat<2;++seat)if(
        (int16_t)(split_cameras[seat][10]|split_cameras[seat][11]<<8)>=2568) {
        split_cameras[seat][0x58]=psx_mod_read_byte(CAMERA+0x58);
        memcpy(split_cameras[seat]+0x94,offsets,sizeof offsets);
    }
    /* EEC00 already wrote the singleton CAMERA. Preserve its old-region
     * source if the world camera belongs to the trailing player. */
    unsigned owner=split_owner?split_owner-1:world_owner;
    if(owner<2 && (int16_t)psx_mod_read_half(CAMERA+10)<2568 && split_cameras[owner][0x58]==6) {
        psx_mod_write_byte(CAMERA+0x58,6);
        project(CAMERA+0x94,split_cameras[owner]+0x94,sizeof offsets);
    }
    psx_mod_counter_add("mmx5.coop.shared-intro-layer-commands",1);
}
static void shared_final_atlas_command(CPUState *cpu,uint32_t address) {
    (void)address;
    /* BIN39's main boss selects the shared layer1 atlas, not a view-local
     * flash: FCE40/FCE60 writes0/100 to CAMERA96 and FCE74 requests refresh.
     * Publish that original command to both retained cameras before either
     * native camera/draw pass consumes it. Never copy a whole camera or
     * advance the shared boss/animation clock for the extra pane. */
    if(!split_available() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY+12)!=12 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800FA000u)!=0x2Bu || cpu->gpr[3]!=CAMERA ||
       psx_mod_read_word(0x800FCE40u)!=0xA4620096u ||
       psx_mod_read_word(0x800FCE60u)!=0xA4400096u ||
       psx_mod_read_word(0x800FCE74u)!=0xA06200A0u)return;
    uint8_t origin[2];capture(CAMERA+0x96,origin,sizeof origin);
    for(unsigned seat=0;seat<2;++seat) {
        memcpy(split_cameras[seat]+0x96,origin,sizeof origin);
        split_cameras[seat][0xA0]=(uint8_t)cpu->gpr[2];
    }
    psx_mod_counter_add("mmx5.coop.shared-final-atlas-commands",1);
}
static void shared_quake_command(CPUState *cpu,uint32_t address) {
    if(!split_available() || cpu->gpr[2]!=CAMERA)return;
    /* Native2A5C4/2A600 finish their shared X/Y quake command here. Publish
     * the new command, before each camera runs its own native update. Copying
     * an already-ticked timer in split_camera_finish would tick P2 twice and
     * invert its phase. Palette flashes are a separate presentation policy. */
    unsigned axis=address==0x8002A638u,mask=axis?1u:16u;
    /* BIN0's three spike-ball landing branches keep the physical ball in
     * s1. Their impact is local to the visible floor, unlike the authored
     * collapse/reveal quakes. Native code still owns the command and phase;
     * publish it only to panes that can see this impact. */
    uint32_t caller=cpu->gpr[31],impact=cpu->gpr[17];
    int local=axis && !scene_owner && !psx_mod_read_byte(PLAY+12) &&
        !psx_mod_read_byte(PLAY+13) && psx_mod_read_word(0x800EE970u)==4 &&
        (caller==0x800EF4D4u || caller==0x800EF670u || caller==0x800EF80Cu) &&
        ((impact>=0x80092090u && impact<0x80093DD0u && !((impact-0x80092090u)%0x9Cu)) ||
         (impact>=0x80096D98u && impact<0x80098118u && !((impact-0x80096D98u)%0x9Cu))) &&
        psx_mod_read_byte(impact);
    int fire_quake=axis && dinorex_fire_content() &&
        (caller==0x800F3DC8u || caller==0x800F3DFCu) &&
        psx_mod_read_word(0x800F3DC0u)==0x0C00A980u &&
        psx_mod_read_word(0x800F3DF4u)==0x0C00A980u;
    static const unsigned fields[]={0x36,0x3A,0x3C,0x3E,0x45};
    for(unsigned seat=0;seat<2;++seat) {
        uint8_t *view=split_cameras[seat];
        if(fire_quake) {
            /* Native warning/burst rumble belongs to the fire route and its
             * immediate upper approach, not the remote opening shaft. */
            int top=(int16_t)(view[0x1A]|view[0x1B]<<8);
            if(!both_living(seat) || top<2192 || top>=3136 ||
               (int16_t)(view[0x28]|view[0x29]<<8)<2416 ||
               (int16_t)(both_body_word(seat,8)>>16)<1137)continue;
        }
        if(local) {
            int x=(int16_t)psx_mod_read_half(impact+10),y=(int16_t)psx_mod_read_half(impact+14);
            int left=(int16_t)(view[10]|view[11]<<8),top=(int16_t)(view[14]|view[15]<<8);
            if(x<left-64 || x>=left+384 || y<top-64 || y>=top+304)continue;
        }
        view[0x34]=(uint8_t)((view[0x34]&~mask)|(cpu->gpr[3]&mask));
        for(unsigned i=0;i<sizeof fields/sizeof fields[0];++i)
            view[fields[i]+axis]=psx_mod_read_byte(CAMERA+fields[i]+axis);
        if(!seat && ai_camera_restore) {
            /* P2-nearest AI restores P1's camera after its borrowed view.
             * Publish this same native command into that pending restore,
             * otherwise split_camera_finish overwrites P1's retained quake
             * with the older canonical bytes. The far-view gate above still
             * applies, and each camera advances the native phase once. */
            ai_camera_restore[0x34]=(uint8_t)((ai_camera_restore[0x34]&~mask)|(cpu->gpr[3]&mask));
            for(unsigned i=0;i<sizeof fields/sizeof fields[0];++i)
                ai_camera_restore[fields[i]+axis]=view[fields[i]+axis];
        }
    }
    psx_mod_counter_add("mmx5.coop.shared-quake-commands",1);
}
static int split_camera_finish(CPUState *cpu,uint32_t address) {
    /* Native mode9 initializes the new checkpoint's camera before3BAC8
     * resets co-op enrollment. A departed stage can still have saved views
     * here; borrowing one overwrites the new room with the old room's bounds.
     * Only gameplay may run private camera follow or publish saved views. */
    if(split_camera_call || psx_mod_local_view_scope() || !split_views || !enrolled || failed ||
       psx_mod_read_byte(PLAY)!=10)return 0;
    split_camera_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    int dead0=life.status[0]==MMX5_COOP_DYING || life.status[0]==MMX5_COOP_FALLEN;
    int dead1=life.status[1]==MMX5_COOP_DYING || life.status[1]==MMX5_COOP_FALLEN;
    if(split_valid && dead0 && dead1) {
        /* With no survivor there is no spectator destination. Keep both
         * last views through their native death effects and retry fade. */
        uint8_t fire_layer[0x54];int fire=dinorex_fire_content();
        if(fire)capture(CAMERA+0x54,fire_layer,sizeof fire_layer);
        project(CAMERA,split_cameras[world_owner],sizeof saved_camera);
        if(fire)project(CAMERA+0x54,fire_layer,sizeof fire_layer);
        split_owner=world_owner+1;split_camera_call=0;
        diagnostics();cpu->gpr[2]=result;return 1;
    }
    uint8_t canonical[0xFC];capture(CAMERA,canonical,sizeof canonical);
    int platform_camera=pegasus_platform_camera();
    if(platform_camera)pegasus_camera_command(canonical);
    int whale_exit=whale_independent_exit();
    if(whale_exit && split_valid) {
        /* The second whale has retired, but its controller retains the room
         * until entry to the draining-water area. Keep the authored Y and
         * bounds while each living hero resumes native horizontal follow. */
        for(unsigned seat=0;seat<2;++seat)if(life.status[seat]==MMX5_COOP_ALIVE) {
            uint8_t *view=split_cameras[seat];
            memcpy(view,canonical,8);
            memcpy(view+12,canonical+12,4);
            memcpy(view+0x18,canonical+0x18,4);
            memcpy(view+0x1C,canonical+0x1C,16);
            view[0x47]=canonical[0x47];
        }
    }
    unsigned owner=world_owner,other=owner^1;
    if(!split_valid) {
        memcpy(split_cameras[0],canonical,sizeof canonical);memcpy(split_cameras[1],canonical,sizeof canonical);
        split_valid=1;
    }
    memcpy(split_cameras[owner],canonical,sizeof canonical);split_owner=owner+1;
    if(life.status[other]==MMX5_COOP_DYING) {
        /* A last survivor's native fatal locks the stage before its life
         * status is published. That lock must not copy its camera over a
         * partner whose death animation is already running. */
    } else if((scene_owner && !duel_fill_pair_present()) || both_door_native_carry() || car_context() ||
       (!scene_owner && life.status[other]==MMX5_COOP_ALIVE &&
        first.body[4]==1 && !first.body[5] && second.body[4]==1 && !second.body[5]) ||
       (whale_authored_camera() && !whale_exit) ||
       (!mmx5_coop_living(&life,other) && life.status[other]!=MMX5_COOP_DYING)) {
        /* A native door transports both bodies under one authored camera
         * command. Following P2 against its previous room's bounds clamps
         * it at the old right edge while P1 continues through, then crushes
         * it on closure. Retain that shared camera through the carry/release;
         * ordinary independent follow resumes when the passage retires.
         * During the pair's initial beams, Dinorex's stage controller owns
         * the downward camera while native follow remains disabled. Share
         * that authored view until either hero finishes its own arrival. */
        memcpy(split_cameras[other],canonical,sizeof canonical);
    } else if(!projected || duel_fill_pair_present()) {
        /* Original primary/parallax updates run once for this extra camera,
         * restoring the canonical camera and all private player projection. */
        memcpy(split_cameras[other],canonical,8);
        shared_atlas_view(split_cameras[other],canonical);
        shared_scroll_view(split_cameras[other],canonical);
        project(CAMERA,split_cameras[other],sizeof canonical);
        glow_stair_view(cpu);
        unsigned was=projected;
        if(was)both_project(other);else enter_second();
        partner_giga_control();
        if(duel_fill_pair_present())psx_mod_write_byte(CAMERA+0x44,1);
        /* Select's outgoing beam is not a camera-follow target. Retain the
         * last gameplay origin while it leaves, rather than following it
         * to the ceiling. Native A6C0 also calls 29184, whose player clamp
         * would pin the beam at Y=0. The layer callbacks still advance their
         * original parallax/animation; 35EDC alone completes departure. */
        /* An incoming beam is presentation too: keep its grounded landing
         * view rather than following its temporary position above the room.
         * Global control/healing freezes do not acquire camera ownership. */
        int departing=life.status[1]==MMX5_COOP_LEAVING ||
            life.status[1]==MMX5_COOP_JOINING;
        uint32_t departing_y=psx_mod_read_word(PLAYER+12);
        if(!departing) {
            intro_climb_camera();
            rose_private_camera_area();
            dinorex_private_camera_focus();
            guest(cpu,0x8002A440,0,0);intro_private_layer_region();
        }
        guest(cpu,0x8002A4E4,0,0);rose_private_layer_region();
        guest(cpu,0x8002A554,0,0);
        if(departing)psx_mod_write_word(PLAYER+12,departing_y);
        capture(CAMERA,split_cameras[other],sizeof canonical);
        if(was)both_project(was);else leave_second();
        project(CAMERA,canonical,sizeof canonical);
        psx_mod_counter_add("mmx5.coop.split-camera-ticks",1);
    }
    split_camera_call=0;diagnostics();cpu->gpr[2]=result;return 1;
}
static unsigned split_regions(PSXPlacementRect regions[2]) {
    Mmx5CoopViewActor actors[2];split_actors(actors);int16_t cameras[2][2];
    for(unsigned seat=0;seat<2;++seat) {
        /* Visibility regions retain the native death effects. AI/scene
         * eligibility still comes from split_actors' living heroes only. */
        if(life.status[seat]==MMX5_COOP_DYING)actors[seat].active=1;
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
static int narrow_lifetime_admission(CPUState *cpu,uint32_t address) {
    unsigned index;uint32_t record=cpu->gpr[4];
    if(!split_scan_call || !split_available() ||
       address!=0x8002AFB4u || cpu->gpr[31]!=0x8002B26Cu ||
       psx_mod_read_word(address)!=0x90820003u ||
       !split_placement_index(record,&index) ||
       (psx_mod_read_byte(record+3)&15u)>=3)return 0;
    unsigned type=psx_mod_read_byte(record+1);
    int dinorex=psx_mod_read_byte(PLAY+12)==4 && psx_mod_read_byte(PLAY+13)==1 &&
        native_armor_overlay() && type==5 &&
        psx_mod_read_word(0x800F7014u)==0x24050020u &&
        psx_mod_read_word(0x800F7018u)==0x0C00B64Eu;
    int necrobat=psx_mod_read_byte(PLAY+12)==2 && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==6 && (type==5 || type==0x28) &&
        psx_mod_read_word(0x800F3DF4u)==0x800F01ECu &&
        psx_mod_read_word(0x800F3E80u)==0x800F2ED8u &&
        psx_mod_read_word(0x800F0194u)==0x24050020u &&
        psx_mod_read_word(0x800F305Cu)==0x24050020u;
    if(!dinorex && !necrobat)return 0;
    int x=(int16_t)psx_mod_read_half(record+4),y=(int16_t)psx_mod_read_half(record+6);
    Mmx5CoopViewActor actors[2];split_actors(actors);
    for(unsigned seat=0;seat<2;++seat)if(actors[seat].active && (!scene_owner || scene_owner==seat+1)) {
        int left=(int16_t)(le32(split_cameras[seat]+8)>>16);
        int top=(int16_t)(le32(split_cameras[seat]+12)>>16);
        if(x>=left-32 && x<left+352 && y>=top-32 && y<top+272)return 0;
    }
    /* The full activation apron is48px, but these native lifetime checks
     * permit32px. A supplemental early spawn retires immediately,
     * leaving its visit latched until both views leave. Wait before native
     * cooldown mutation/allocation; original AI, draw and retirement remain
     * unchanged once either view can retain the actual actor. */
    psx_mod_counter_add(dinorex?"mmx5.coop.dinorex-bat-admission-waits":
        type==5?"mmx5.coop.necrobat-bat-admission-waits":
                "mmx5.coop.necrobat-spike-admission-waits",1);
    cpu->gpr[2]=1;return 1;
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
    /* Training22 uses the same native placement records as the campaign.
     * Admit its enemies and camera areas from either living view too. */
    if((stage>=19 && stage!=22) || section>1)return 0;
    uint32_t table=psx_mod_read_word(0x80072EACu+stage*8u+section*4u);
    /* Records are eight bytes, but original BIN1's Grizzly table FBD3C is
     * only word aligned. Rejecting it skips the two-view activation union. */
    if((table&3u) || table<0x80010000u || table>=0x80200000u)return 0;
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
    /* The dying seat still owns a visible pane. Culling only against living
     * heroes deletes its random death glyphs at their first native update. */
    for(unsigned seat=0;seat<2;++seat)
        if(life.status[seat]==MMX5_COOP_DYING)actors[seat].active=1;
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
    /* Sigma2's lower ladder gate is only16px wide and starts at Y1712.
     * Zero can jump over that rectangle from the checkpoint platform. The
     * adjoining authored shaft runs to the upper gate at Y224; admit that
     * same minY0 command throughout its interior, including after Continue.
     * The hero still waiting on the checkpoint platform retains its bounds. */
    if(actor.active && parameters==0x8007D338u &&
       psx_mod_read_byte(PLAY+12)==17 && !psx_mod_read_byte(PLAY+13) &&
       psx_mod_read_word(0x800EE970u)==31 &&
       psx_mod_read_word(parameters)==0x10FF10F0u &&
       actor.x>=4336 && actor.x<=4511 && actor.y>=224 && actor.y<=1871)return 1;
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
static void dinorex_native_camera_focus(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!cpu || !dinorex_fire_stage() || psx_mod_local_view_scope() ||
       cpu->gpr[3]!=0x800A0000u || cpu->gpr[2]!=192u ||
       psx_mod_read_word(0x800F414Cu)!=0x240200C0u)return;
    /* The one shaft tracker retires when either hero activates checkpoint1.
     * Its singleton focus write can reach a hero still in the upper shaft.
     * Keep that lifecycle, but let the private admission below apply the
     * same focus/rebase to each view at its own lower-room boundary. */
    cpu->gpr[2]=psx_mod_read_half(CAMERA+0x2E);
    psx_mod_counter_add("mmx5.coop.dinorex-private-focus-admissions",1);
}
static void dinorex_private_camera_focus(void) {
    if(!dinorex_fire_stage() || scene_owner || psx_mod_local_view_scope() ||
       !psx_mod_read_byte(PLAY+0x1D) || psx_mod_read_byte(PLAYER+4)!=1 ||
       !(psx_mod_read_byte(PLAYER+0x5C)&127u) ||
       (int16_t)psx_mod_read_half(PLAYER+14)<2228 ||
       psx_mod_read_word(0x800F414Cu)!=0x240200C0u ||
        psx_mod_read_word(0x800F4150u)!=0xA462A226u ||
        psx_mod_read_word(0x800F5914u)!=0x00800080u)return;
    /* Admit the192px focus only after this view's original lower-room
     * bounds have committed. Changing it while the shaft still has minY512
     * moves the camera above2192 and prevents EEAF0's entry predicate from
     * ever acquiring its2192/2192 lock. The leader's checkpoint alone does
     * not establish that the other view has entered. */
    int committed=(int16_t)psx_mod_read_half(CAMERA+0x22)>=2192 &&
        (int16_t)psx_mod_read_half(CAMERA+0x2A)>=2192;
    int focus=committed?(int16_t)psx_mod_read_half(0x800F414Cu):
                        (int16_t)psx_mod_read_half(0x800F5914u);
    /* Preserve the native128px shaft focus if an earlier singleton command
     * reached this view before its lower bounds. This also lets the original
     * entry predicate recover an already elevated view, without setting its
     * bounds or checkpoint ourselves. Zero's grounded origin is one pixel
     * above X's on this floor; compensate the lower follow offset so the
     * platform itself has equal framing in both panes. */
    if(committed && psx_mod_read_byte(PLAYER+2)==1)--focus;
    int previous=(int16_t)psx_mod_read_half(CAMERA+0x2E);
    if(previous!=focus) {
        /* Preserve the tracked point when admitting the changed offset.
         * Native follow has a dead zone: changing only2E after arrival can
         * retain the old lower origin indefinitely, even after a jump. */
        int32_t before=(int32_t)psx_mod_read_word(CAMERA+0x18);
        int64_t y=(int64_t)before+(previous-focus)*65536;
        int64_t minimum=(int16_t)psx_mod_read_half(CAMERA+0x22)*65536;
        int64_t maximum=(int16_t)psx_mod_read_half(CAMERA+0x20)*65536;
        if(y<minimum)y=minimum;
        if(y>maximum)y=maximum;
        psx_mod_write_word(CAMERA+12,psx_mod_read_word(CAMERA+12)+(uint32_t)(y-before));
        psx_mod_write_word(CAMERA+0x18,(uint32_t)y);
        psx_mod_write_half(CAMERA+0x2E,(uint16_t)focus);
    }
}
static void rose_private_camera_area(void) {
    /* Native55F60 retires the high-bit area5 actor after its shared atlas
     * request. A trailing body still needs that original room command when
     * it reaches the authored rectangle; replay only the private bounds,
     * never the retired actor, atlas request or shared stage producer. */
    if(!split_available() || scene_owner || car_context() ||
       !(psx_mod_read_byte(PLAYER+0x5C)&127) ||
       psx_mod_read_byte(PLAY+12)!=7 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=14 ||
       psx_mod_read_word(0x80056070u)!=0xAC6292F8u ||
       psx_mod_read_word(0x800E92F8u)!=2 ||
       (psx_mod_read_word(0x800FA680u)&0x00FFFFFFu)!=0x00850001u ||
       psx_mod_read_word(0x8007D028u)!=0x111F10E0u ||
       psx_mod_read_word(0x8007D02Cu)!=0x014F00F0u ||
       psx_mod_read_word(0x8007D030u)!=0x10401040u ||
       psx_mod_read_word(0x8007D034u)!=0x03C000F0u)return;
    Mmx5CoopViewActor body={.active=1,
        .x=(int16_t)psx_mod_read_half(PLAYER+10),
        .y=(int16_t)psx_mod_read_half(PLAYER+14)};
    if(camera_area_contains(0x8007D028u,body)) {
        uint8_t view[0xFC];capture(CAMERA,view,sizeof view);
        camera_area_apply(0x8007D028u,view);project(CAMERA,view,sizeof view);
    }
}
static void share_hint_knowledge(uint32_t actor) {
    unsigned index=psx_mod_read_byte(actor+2);
    unsigned character=psx_mod_read_byte(PLAYER+2),campaign=psx_mod_read_byte(PLAY+0x44);
    if(index>=54 || character>=2 || campaign>=2)return;
    unsigned bit=1u<<(index&7u),offset=0xCDu+index/8u;
    /* Scripts consult campaign knowledge; native admission records the
     * speaking body. Copy only a bit the original actor actually admitted. */
    if(psx_mod_read_byte(PLAY+offset+character*32u)&bit)
        psx_mod_write_byte(PLAY+offset+campaign*32u,
            (uint8_t)(psx_mod_read_byte(PLAY+offset+campaign*32u)|bit));
}
static int spatial_stage_actor(CPUState *cpu,uint32_t address) {
    if(spatial_call || car_context() || !split_available())return 0;
    uint32_t actor=cpu->gpr[4];
    if(actor<0x8009E7A0u || actor>=0x8009F9A0u || (actor-0x8009E7A0u)%0x30u)return 0;
    unsigned checkpoint=address==0x80056AA0u,hint=address==0x80056E88u;
    if(psx_mod_read_byte(actor+1)!=(checkpoint?6u:hint?14u:0u))return 0;
    if(projected) {
        if(!hint || !world_owner)return 0;
        spatial_call=1;uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
        share_hint_knowledge(actor);spatial_call=0;cpu->gpr[2]=result;return 1;
    }
    Mmx5CoopViewActor actors[2];split_actors(actors);Mmx5CoopViewActor eligible[2]={actors[0],actors[1]};
    unsigned hint_index=psx_mod_read_byte(actor+2);
    uint32_t parameters=hint?(hint_index<54?0x8007D824u+hint_index*8u:0):
        psx_mod_read_byte(actor+4)?psx_mod_read_word(actor+0x18):0;
    for(unsigned seat=0;seat<2;++seat) {
        if(!checkpoint && parameters)eligible[seat].active=(unsigned)camera_area_contains(parameters,actors[seat]);
        if(checkpoint)eligible[seat].active=actors[seat].active &&
            ((psx_mod_read_byte(actor+2)&0x80)?actors[seat].y>=(int16_t)psx_mod_read_half(actor+14):
                                             actors[seat].x>=(int16_t)psx_mod_read_half(actor+10));
    }
    unsigned target=scene_owner?scene_owner-1:mmx5_coop_view_nearest(
        eligible[0].active || eligible[1].active?eligible:actors,
        (int16_t)psx_mod_read_half(actor+10),(int16_t)psx_mod_read_half(actor+14),0);
    /* Rose's native CP3 is X-only and can reappear in the trailing upper
     * view after the living partner has already admitted CP7's lower room.
     * Keep that earned retry point in this exact split-route case; still
     * run56B28 and its original2D760 retirement. Native backtracking and
     * every other checkpoint retain their original replacement policy. */
    unsigned partner=target^1u;
    int retain_rose_cp7=checkpoint && psx_mod_read_byte(PLAY+0x1D)==7 &&
        psx_mod_read_byte(PLAY+12)==7 && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==14 &&
        psx_mod_read_word(0x80056B88u)==0xA0621C1Du &&
        psx_mod_read_word(actor+0x10)==0x800FA630u &&
        psx_mod_read_byte(actor+2)==3 &&
        actors[target].x>=4304 && actors[target].x<4480 &&
        actors[target].y>=240 && actors[target].y<=431 &&
        life.status[partner]==MMX5_COOP_ALIVE && (actor_byte(partner,0x5C)&127u) &&
        actors[partner].y>=1008 &&
        le32(split_cameras[partner]+0x24)==0x10401E20u &&
        le32(split_cameras[partner]+0x28)==0x03C003C0u;
    uint8_t canonical[0xFC];capture(CAMERA,canonical,sizeof canonical);spatial_call=1;
    uint8_t campaign=psx_mod_read_byte(PLAY+0x44);
    if(target) {
        project(CAMERA,split_cameras[target],sizeof canonical);enter_second();
        /* Camera areas can queue shared scenery/palette changes; checkpoints
         * and hints also retain the one campaign. Only their body/camera
         * predicate belongs to P2, not the whole-stage resource buffer. */
        psx_mod_write_byte(PLAY+0x44,campaign);psx_mod_write_word(SCRATCH+0x28,resources[3]);
        if(hint)world_owner=1;
    }
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    if(retain_rose_cp7 && psx_mod_read_byte(PLAY+0x1D)==3)
        psx_mod_write_byte(PLAY+0x1D,7);
    if(hint)share_hint_knowledge(actor);
    if(target) {
        capture(CAMERA,split_cameras[target],sizeof canonical);if(hint)world_owner=0;leave_second_context(1);
        if(scene_owner!=2)project(CAMERA,canonical,sizeof canonical);
    }
    /* Native55F60 still admits room bounds during the door's shared hold.
     * Once both heroes are in the original carry, publish that command to
     * each body inside its authored rectangle. Restoring a P2-scoped native
     * call otherwise discards P1's arena bounds before the entrance drop.
     * A partner still waiting outside keeps its independent view. */
    int paired_passage=both_door.phase==2 && both_door_native_carry();
    if(!hint && !checkpoint && !scene_owner &&
       (paired_passage || (!psx_mod_read_byte(PLAY+0x10) && !psx_mod_read_byte(PLAY+0x1C)))) {
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
static int dark_hold_palette(CPUState *cpu,uint32_t address) {
    static unsigned calling;
    if(calling || !enrolled || failed || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=10)return 0;
    uint32_t palette=psx_mod_read_word(SCRATCH+0x28);
    uint8_t held=psx_mod_read_byte(PLAYER+0xF7);
    /* Native2C2C0 builds the inverted stage CLUT and2BD8C selects it for
     * upload. The shared draw must see either hero's actual Dark Hold flag
     * and the committed stage palette, rather than a character-only bank. */
    if(projected)psx_mod_write_word(SCRATCH+0x28,resources[3]);
    PlayerContext *borrowed=NULL;
    if(!held) {
        PlayerContext *other=projected?&first:&second;
        if(other->body[0xF7])for(unsigned i=0;i<sizeof other->shots;i+=0x9Cu)
            if(other->shots[i] && other->shots[i+1]==0x21 &&
               other->shots[i+4]==2) {
                /* The native palette selector reads singleton PLAYER.F7.
                 * Borrow the actual attack owner's flag only for its draw
                 * update; never apply a held status to the other hero. */
                psx_mod_write_byte(PLAYER+0xF7,other->body[0xF7]);borrowed=other;break;
            }
    }
    calling=1;uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);calling=0;
    if(borrowed && address==0x8002C2C0u)
        borrowed->body[0xF7]=psx_mod_read_byte(PLAYER+0xF7);
    psx_mod_write_byte(PLAYER+0xF7,held);psx_mod_write_word(SCRATCH+0x28,palette);
    cpu->gpr[2]=result;return 1;
}
static uint16_t retained_palette_color(uint16_t color,unsigned row) {
    /* Resident2BD8C applies stage fades/white flashes at palette upload,
     * leaving both retained source palettes untouched. Mirror its selected
     * row mask and saturating RGB offsets for immutable sprite banks too.
     * Zero remains transparent; preserve the native semitransparency bit. */
    unsigned rows=psx_mod_read_byte(PLAY+12)==11?0x200u:0x140u;
    if(!color || row>=rows ||
       !(psx_mod_read_word(0x80093DD0u+(row/32u)*4u)&(0x80000000u>>(row&31u))))return color;
    const uint32_t channels[3]={0x800D50DCu,0x8009A42Cu,0x80093E66u};
    const unsigned masks[3]={0x1Fu,0x3E0u,0x7C00u};
    uint16_t result=color&0x8000u;
    for(unsigned i=0;i<3;++i) {
        unsigned value=color&masks[i],delta=psx_mod_read_half(channels[i]+2u);
        if(psx_mod_read_byte(channels[i]))value=value<delta?0:value-delta;
        else {value+=delta;if(value>masks[i])value=masks[i];}
        result|=(uint16_t)value;
    }
    return result;
}
static uint16_t sprite_bank(uint32_t table,unsigned frame,uint16_t clut,uint16_t tile) {
    unsigned *used=psx_mod_local_view_scope()?&view_bank_count:&bank_count;
    SpriteBank *cache=psx_mod_local_view_scope()?view_banks:banks;
    CharacterAssets *a=duel_render_art?duel_render_art:&assets[counterpart];
    uint8_t native_compressed[65536];
    const uint8_t *compressed;size_t available;uint32_t packed,source;
    if(clut<0x7800)return 0;
    if(render_primary && !duel_render_art) {
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
    unsigned tiles=packed>>20;size_t palette_offset;
    if(!tiles || tiles>256 ||
       !mmx5_coop_duel_palette_offset(clut,1,0x4000,&palette_offset))return 0;
    uint32_t palette=render_primary && !duel_render_art?psx_mod_read_word(SCRATCH+0x28):a->palette;
    uint8_t colors[32];unsigned npc_row;
    if(duel_render_art && mmx5_coop_duel_x_palette_row(clut,&npc_row))
        memcpy(colors,story_duel.colors+npc_row*32,sizeof colors);
    else capture(palette+(uint32_t)palette_offset,colors,sizeof colors);
    for(unsigned i=0;i<16;++i) {
        uint16_t color=retained_palette_color((uint16_t)(colors[i*2]|colors[i*2+1]<<8),
                                               (unsigned)(palette_offset/32u));
        colors[i*2]=(uint8_t)color;colors[i*2+1]=(uint8_t)(color>>8);
    }
    for(unsigned i=0;i<(*used);++i)
        if(cache[i].source==source && cache[i].tile==tile &&
           cache[i].generation==(render_primary && !duel_render_art?asset_generation:a->record) &&
           !memcmp(cache[i].colors,colors,sizeof colors))return cache[i].id;
    if(render_primary && !duel_render_art)capture(source,native_compressed,available);
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
        mmx5_coop_sprite_tile_pixels(pixels,tile,local);
        output=local;count=256;
    }
    uint32_t hash=2166136261u;
    for(unsigned i=0;i<count;++i)hash=(hash^output[i])*16777619u;
    unsigned index=(*used);
    for(unsigned i=0;i<(*used);++i)
        if(cache[i].pixel_hash==hash && cache[i].pixel_count==count &&
           !memcmp(cache[i].pixels,output,count*2)) {index=i;break;}
    if(index==(*used)) {
        unsigned first_id=psx_mod_local_view_scope()?0xA000u:0x6000u;
        index=mmx5_coop_texture_cache_slot(*used,(uint16_t)first_id,
            psx_mod_read_word(PLAY+0x1B4),arena,primary_arena,hud_arena,psx_mod_read_word);
        if(index>=4096) {psx_mod_counter_add("mmx5.coop.sprite-cache-pinned",1);return 0;}
        uint16_t *copy=malloc(count*2);if(!copy)return 0;memcpy(copy,output,count*2);
        uint16_t id=(uint16_t)(first_id+index);
        unsigned replacing=index<*used;
        if(replacing && !psx_mod_forget_texture_bank_range(id,1)) {free(copy);return 0;}
        if(!psx_mod_define_texture_bank(id,tile==UINT16_MAX?256:16,tile==UINT16_MAX?256:16,output)) {
            if(replacing) {free(cache[index].pixels);memset(&cache[index],0,sizeof cache[index]);}
            free(copy);return 0;
        }
        if(replacing)free(cache[index].pixels);
        cache[index].id=id;cache[index].pixels=copy;cache[index].pixel_hash=hash;cache[index].pixel_count=count;
        if(!replacing)++(*used);else psx_mod_counter_add("mmx5.coop.sprite-cache-reuses",1);
    }
    cache[index].source=source;cache[index].tile=tile;
    cache[index].generation=render_primary && !duel_render_art?asset_generation:a->record;
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
static int sprite_outside(uint32_t actor,unsigned selector) {
    if(!split_views || !enrolled || failed || psx_mod_read_byte(PLAY)!=10)return 0;
    unsigned layer=psx_mod_read_byte(actor+selector);
    if(layer>=3)return 0; /* Screen-relative UI/script art. */
    uint32_t camera=CAMERA+layer*0x54u;
    int x=(int16_t)(psx_mod_read_half(actor+10)-psx_mod_read_half(camera+10));
    int y=(int16_t)(psx_mod_read_half(actor+14)-psx_mod_read_half(camera+14));
    int margin=psx_mod_widescreen_x_margin();
    /* Native assembly offsets are signed bytes with16px tiles. Preserve a
     * conservative144px sprite border, then cull before GP0 truncates XY to
     * eleven bits. Visibility/lifetime still belong to the two-camera union. */
    return x < -144-margin || x > 320+144+margin || y < -144 || y > 240+144;
}
/* BIN0's staged counterpart is a shared effect actor, not either hero.
 * F81BC allocates class3A; F15AC selects its X/Zero animation and leaves
 * native dialogue/PLAY28 completion in its update. Hide only its packets
 * during co-op, including both private views and a fallen partner scene. */
static int intro_counterpart_draw(uint32_t actor) {
    if(!enrolled || failed || psx_mod_read_byte(PLAY)!=10 ||
       psx_mod_read_byte(PLAY+0x0C) || psx_mod_read_byte(PLAY+0x0D) ||
       psx_mod_read_word(0x800EE970u)!=4u ||
       psx_mod_read_word(0x800FA6F4u)!=0x800F15ACu ||
       psx_mod_read_word(0x800F15ACu)!=0x27BDFFE0u ||
       actor<0x800D1F40u || actor>=0x800D4F40u ||
       (actor-0x800D1F40u)%0x60u)return 0;
    return psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==0x3Au &&
        !psx_mod_read_byte(actor+2) &&
        psx_mod_read_word(actor+0x30)==0x800FAA14u;
}
static int cull_sprite_draw(CPUState *cpu,uint32_t address) {
    if(psx_mod_local_view_scope() && sigma1_beam_cleared(cpu->gpr[4],local_view_seat)) {
        cpu->gpr[2]=0;return 1;
    }
    /* PLAYER's retained painter emits X's complete body and armor together.
     * The later singleton armor pass samples live VRAM, which can contain
     * Zero while X is the returning passenger of a P2-owned hint. */
    if(address==0x80027370u && cpu->gpr[4]==PLAYER && enrolled && !failed &&
       psx_mod_read_byte(PLAY)==10 && psx_mod_texture_banks_supported()) {
        cpu->gpr[2]=0;return 1;
    }
    if(duel_authored_draw(cpu->gpr[4])) {cpu->gpr[2]=0;return 1;}
    if(address==0x80024C60u && intro_counterpart_draw(cpu->gpr[4])) {
        cpu->gpr[2]=0;return 1;
    }
    /* The retained body painter already appended this cosmetic armor once. */
    if(story_duel.phase==4 && duel_scope() && cpu->gpr[4]==story_duel.opponent) {
        cpu->gpr[2]=0;return 1;
    }
    if(address==0x80027370u && cpu->gpr[4]==PLAYER && story_duel.phase>=2 &&
       duel_scope() && !psx_mod_read_byte(PLAYER+2) &&
       (projected?1u:0u)==(story_duel.entrant^1u)) {cpu->gpr[2]=0;return 1;}
    if(!sprite_outside(cpu->gpr[4],address==0x80024C60u || address==0x80027370u?0x14u:0x37u))return 0;
    psx_mod_counter_add(address==0x80027370u?"mmx5.coop.split-distant-armor-culled":
                        "mmx5.coop.split-distant-sprite-culled",1);
    cpu->gpr[2]=0;return 1;
}
static int private_static_effect(uint32_t actor) {
    if(actor<0x80094398u || actor>=0x80096D98u || (actor-0x80094398u)%0x70u ||
       !psx_mod_read_byte(actor))return 0;
    unsigned owner=personal_effect_owner[(actor-0x80094398u)/0x70u];
    if(owner && psx_mod_read_word(actor+0x38))return 0;
    uint32_t assembly=psx_mod_read_word(actor+0x3C);
    uint32_t root=owner?assets[counterpart].assembly:
        projected?resources[1]:psx_mod_read_word(SCRATCH+0x1C);
    /* Static hit sparks and dash exhaust retain the allocating character's
     * assembly, but have no compressed frame pointer. Their shared update
     * already runs under that owner; drawing also needs its retained image. */
    return assembly>=root && assembly-root<0xC000u?(int)owner+1:0;
}
static void render_actor_piece(CPUState *cpu,uint32_t actor,uint32_t base_arena,int armor) {
    if(sprite_outside(actor,0x14u))return;
    uint32_t base=psx_mod_read_word(SCRATCH+0x100),mode=psx_mod_read_word(SCRATCH+0x104);
    uint32_t table=psx_mod_read_word(actor+0x38),assembly=psx_mod_read_word(actor+0x3C);
    unsigned frame=psx_mod_read_byte(actor+0x47);
    const uint8_t *owner=render_primary?first.body:second.body;
    if(!table && assembly==le32(owner+0x3C)) {
        table=le32(owner+0x38);frame=owner[0x47];
    }
    unsigned image_character=render_primary?first.body[2]:counterpart;
    CharacterAssets *a=duel_render_art?duel_render_art:&assets[image_character];
    /* Z-Buster3A, Gaea shot2D and dash exhaust use native image tiles, not
     * compressed body frames. Preserve their authored VRAM origin even when
     * their flags use the ordinary tile-number layout (Gaea's tile0520).
     * Only owner-bound character assemblies qualify; stage images stay native. */
    uint32_t character_assembly=render_primary && !duel_render_art?
        psx_mod_read_word(SCRATCH+0x1C):a->assembly;
    int static_image=!table && assembly>=character_assembly &&
        assembly-character_assembly<0xC000u;
    /* Native46AF0 takes charged Crescent's assembly from directory68 and
     * draws its two class0A halves at tile0540. Native50D04/subC installs
     * graphics directory34 there. This sheet is compressed, absent from the
     * immutable NIM atlas, and cannot be read from Zero's live VRAM. Bind it
     * to the producer so a later weapon selection cannot change a live shield. */
    /* F-Laser's class6 assembly encodes its steering direction, beyond
     * the31 identical graphics entries in directory1C. Its native image is
     * one shared32-tile sheet, so do not index compressed data by that pose. */
    if(!image_character && actor>=SHOTS && actor<SHOTS+sizeof second.shots &&
       !((actor-SHOTS)%0x9Cu) && psx_mod_read_byte(actor+1)==6) {
        uint32_t graphics=render_primary?psx_mod_read_word(SCRATCH+0x14):a->sprites;
        uint32_t image=render_primary?psx_mod_read_word(graphics+0x1C):le32(a->graphics+0x1C);
        if(table==graphics+image)frame=0;
    }
    unsigned image_directory=0;
    if(!table && !image_character && actor>=SHOTS && actor<SHOTS+sizeof second.shots &&
       !((actor-SHOTS)%0x9Cu)) {
        uint32_t root=render_primary?psx_mod_read_word(SCRATCH+0x1C):a->assembly;
        unsigned kind=psx_mod_read_byte(actor+1);
        if(kind==0x0A && assembly==root+le32(a->graphics+0x40068))image_directory=0x34;
        /* Charged Goo Shaver's eight class0C actors use assembly64 at
         * tile0540 (native471F8). The original upload50D04/subB installs
         * compressed graphics30 there, rather than an immutable NIM tile. */
        if(kind==0x0C && assembly==root+le32(a->graphics+0x40064))image_directory=0x30;
        /* Native4981C binds charged Tri Thunder (class0E) to assembly6C
         * at the same tile0540;50D04/subD uploads compressed graphics38. */
        if(kind==0x0E && assembly==root+le32(a->graphics+0x4006C))image_directory=0x38;
    }
    if(image_directory)static_image=1;
    if(static_image && render_primary)
        psx_mod_counter_add("mmx5.coop.p1-retained-static",1);
    uint16_t native_tile=psx_mod_read_half(actor+0x40);
    int relative=!static_image && !(psx_mod_read_byte(actor)&0x60u);
    /* Unremapped native frames upload at the header's low-byte tile slot.
     * Retained decoded atlases begin at tile zero. Let the original painter
     * emit relative UVs, including its flipped-edge geometry, then restore
     * the native header. Alternate native atlas layouts keep their paths. */
    if(relative)psx_mod_write_half(actor+0x40,native_tile&0xFF00u);
    guest(cpu,armor>=0?0x80026CACu:0x80024C60u,actor,armor>=0?(unsigned)armor:0);
    if(relative)psx_mod_write_half(actor+0x40,native_tile);
    uint32_t end=psx_mod_read_word(SCRATCH+0x100);
    if(end<base || end-base_arena>40000 || (end-base)%40) {
        psx_mod_write_word(diagnostic+0xF00,1);psx_mod_write_word(diagnostic+0xF04,base);
        psx_mod_write_word(diagnostic+0xF08,end);psx_mod_write_word(diagnostic+0xF0C,(uint32_t)armor);
        failed=1;return;
    }
    if((!render_primary || duel_render_art) && !static_image &&
       (table<a->sprites || table-a->sprites>=a->graphics_bytes))return;
    if(render_primary && !duel_render_art && !static_image &&
       (!table || table<0x80010000u || table>=0x80200000u))return;
    uint32_t expanded=base_arena+40000+(base-base_arena)*2;
    for(uint32_t at=base;at<end;at+=40,expanded+=80,mode+=8) {
        uint32_t q[10];for(unsigned i=0;i<10;++i)q[i]=psx_mod_read_word(at+i*4);
        uint16_t static_page=(uint16_t)(q[5]>>16),static_uv=(uint16_t)q[3];
        unsigned reflection=0;
        uint16_t tile;int supported=mmx5_coop_sprite_quad(q,&tile);
        /* SPRT_16 conversion replaces its absolute UV and has no FT4 TPAGE.
         * Sample the original tile with its preceding native draw mode. */
        if(tile!=UINT16_MAX) {static_uv=tile;static_page=(uint16_t)psx_mod_read_word(mode+4);}
        if(tile!=UINT16_MAX)q[5]|=(psx_mod_read_word(mode+4)&0x60u)<<16;
        if(supported && (relative || static_image) && tile==UINT16_MAX) {
            tile=mmx5_coop_sprite_quad_tile(q);
            if(static_image) {
                /* Native flipped FT4 endpoints can precede their authored
                 * image tile by one pixel, and page-edge clipping can shrink
                 * the geometry to15px. Taking the minimum UV samples the
                 * preceding row/column. Class15's lingering plasma orb exposes
                 * these seams. Retain the real tile, reflect its pixels and
                 * paint the complete16px square, like compressed body tiles. */
                static_uv=tile&0xF0F0u;reflection=tile&3u;
            }
        }
        uint16_t bank=!supported?0:static_image?
            retained_tile_bank(static_page,(uint16_t)(q[3]>>16),static_uv,render_primary?2:1,reflection,image_directory,image_character):
            sprite_bank(table,frame,(uint16_t)(q[3]>>16),tile);
        if(!bank) {
            /* Failure-only evidence for a rejected native sprite: retain
             * its exact packet and source, without altering gameplay. */
            psx_mod_write_word(diagnostic+0xF00,4);
            psx_mod_write_word(diagnostic+0xF04,actor);
            psx_mod_write_word(diagnostic+0xF08,table);
            psx_mod_write_word(diagnostic+0xF0C,image_character|(render_primary<<8)|
                ((unsigned)static_image<<16)|((unsigned)supported<<24));
            project(diagnostic+0xF10,q,sizeof q);
            psx_mod_write_word(diagnostic+0xF38,frame);
            psx_mod_write_word(diagnostic+0xF3C,assembly);
            psx_mod_write_word(diagnostic+0xF40,tile);
            failed=1;psx_mod_counter_add("mmx5.coop.sprite-rejected",1);return;
        }
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
    if(actor==PLAYER && !render_primary && psx_mod_local_view_scope())
        psx_mod_counter_add("mmx5.coop.local-view-p2-body-quads",(end-base)/40);
}
static void render_actor(CPUState *cpu,uint32_t actor,uint32_t base_arena) {
    /* Both duel participants use their real selected form and normal native
     * animation/weapon palettes. Reuse the ordinary retained painter; an NPC
     * costume cannot safely index Falcon/weapon-only animation families. */
    render_actor_piece(cpu,actor,base_arena,-1);
    if(actor==PLAYER && !psx_mod_read_byte(actor+2)) {
        /* Native26CAC indexes the global assembly directory, unlike the
         * base-body painter which follows actor3C. P1 may be Zero. A
         * P1/X also needs these pieces in private redraws: the singleton
         * native armor painter otherwise reads Zero's most recent VRAM
         * upload during an Alia return. Beam frames include armor pieces
         * as well as the compressed body. Keep the whole owner together. */
        uint32_t sprites=psx_mod_read_word(SCRATCH+0x14),assembly=psx_mod_read_word(SCRATCH+0x1C);
        if(!render_primary) {
            psx_mod_write_word(SCRATCH+0x14,assets[0].sprites);
            psx_mod_write_word(SCRATCH+0x1C,assets[0].assembly);
        }
        unsigned groups[4],count=mmx5_coop_armor_layers(psx_mod_read_byte(actor+0xFA),
            psx_mod_read_byte(actor+0x155),psx_mod_read_byte(actor+5),psx_mod_read_byte(actor+0x47),groups);
        for(unsigned i=0;i<count && !failed;++i)render_actor_piece(cpu,actor,base_arena,(int)groups[i]);
        psx_mod_write_word(SCRATCH+0x14,sprites);psx_mod_write_word(SCRATCH+0x1C,assembly);
    }
}
static int paired_chaser_draw(void) {
    return enrolled && !failed && car_context() &&
        life.status[0]==MMX5_COOP_ALIVE && life.status[1]==MMX5_COOP_ALIVE &&
        first.body[3] && second.body[3] &&
        first.body[0xD5]==0xFF && second.body[0xD5]==0xFF &&
        first.car[0] && second.car[0] && first.car[3] && second.car[3] &&
        first.car[4]==1 && second.car[4]==1;
}
static unsigned chaser_exhaust_owner(uint32_t actor) {
    if(actor<0x80094398u || actor>=0x80096D98u || (actor-0x80094398u)%0x70u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x0D)return 0;
    uint32_t parent=psx_mod_read_word(actor+0x50);
    for(unsigned seat=0;seat<2;++seat)if(parent==car_mirror[seat])return seat+1;
    return 0;
}
static uint32_t paired_armor_rider(unsigned seat) {
    if(!enrolled || failed || !native_armor_overlay() ||
       life.status[0]!=MMX5_COOP_ALIVE || life.status[1]!=MMX5_COOP_ALIVE ||
       !first.body[3] || !second.body[3])return 0;
    const uint8_t *body=seat?second.body:first.body;
    if(body[0xD5]!=0xFFu && body[0xD5]!=0xFEu)return 0;
    uint32_t armor=CAR+(255u-body[0xD5])*0xB0u;
    return body[4]==1 &&
        psx_mod_read_byte(armor) && psx_mod_read_byte(armor+1)==1 &&
        psx_mod_read_byte(armor+3) && (psx_mod_read_byte(armor+0x97)&0x40u)?armor:0;
}
static void render_paired_armor(CPUState *cpu,unsigned seat) {
    uint32_t armor=paired_armor_rider(seat);if(!armor)return;
    /* Native armor priority3 sorts behind both priority2 heroes. Keep the
     * occupied suit immediately before its own rider, preserving that native
     * internal order while the complete P1 pair follows the complete P2 pair.
     * Idle suits and the native simulation/snapshot bytes stay unchanged. */
    uint8_t priority=psx_mod_read_byte(armor+0x16);
    psx_mod_write_byte(armor+0x16,psx_mod_read_byte(PLAYER+0x16));
    guest(cpu,0x80024C60u,armor,0);
    psx_mod_write_byte(armor+0x16,priority);
}
static void render_paired_chaser(CPUState *cpu,unsigned seat) {
    if(!psx_mod_read_byte(CAR+3))return;
    /* Native bodies use priority2 and bikes4, which sorts both riders over
     * both machines. Append each bike immediately before its own rider in
     * the rider's existing gameplay bucket: P2 bike/body, then P1 bike/body.
     * Restore the actor byte before any simulation or snapshot can observe
     * it; vehicle physics, stage art and UI priorities stay native. */
    uint8_t priority;
    /* The persistent native exhaust changes with its bike's dash. Leaving
     * it in the later shared effect pass draws P2's flame over P1's whole
     * group. Native exhaust6 also belongs behind its machine4: append it
     * BEFORE the bike when both use the rider's bucket2. Shots, scenery,
     * HUD and unrelated effects retain their native ordering. */
    for(unsigned i=0;i<96;++i) {
        uint32_t actor=0x80094398u+i*0x70u;
        if(chaser_exhaust_owner(actor)!=seat+1 || !psx_mod_read_byte(actor+3))continue;
        priority=psx_mod_read_byte(actor+0x16);
        psx_mod_write_byte(actor+0x16,psx_mod_read_byte(PLAYER+0x16));
        guest(cpu,0x80024C60,actor,0);psx_mod_write_byte(actor+0x16,priority);
    }
    priority=psx_mod_read_byte(CAR+0x16);
    psx_mod_write_byte(CAR+0x16,psx_mod_read_byte(PLAYER+0x16));
    guest(cpu,0x80024C60,CAR,0);psx_mod_write_byte(CAR+0x16,priority);
}
static int paired_second_buster_effect(uint32_t actor) {
    if(!enrolled || failed || life.status[0]!=MMX5_COOP_ALIVE ||
       life.status[1]!=MMX5_COOP_ALIVE || !first.body[3] || !second.body[3] ||
       actor<0x80094398u || actor>=0x80096D98u ||
       (actor-0x80094398u)%0x70u ||
       !personal_effect_owner[(actor-0x80094398u)/0x70u])return 0;
    if(!psx_mod_read_byte(actor) || !psx_mod_read_byte(actor+3))return 0;
    unsigned kind=psx_mod_read_byte(actor+1);
    if(kind==3 && psx_mod_read_byte(actor+2)==0x0E)return 1;
    /* X's native class2 emission particles accompany all three buster
     * strengths; class5 follows a real private charge-shot actor. They are
     * character cosmetics, separate from shared hit sparks and SHOTS. */
    uint32_t parent=psx_mod_read_word(actor+0x50);
    return !counterpart && (kind==2 || (kind==5 && parent>=SHOTS &&
        parent<SHOTS+sizeof second.shots && !((parent-SHOTS)%0x9Cu)));
}
static void render_second_buster_effects(CPUState *cpu,uint32_t base) {
    /* Personal muzzle/emission effects share their owner's body group.
     * Shared impact sparks keep their native world priority. */
    for(unsigned i=0;i<96;++i) {
        uint32_t actor=0x80094398u+i*0x70u;
        if(!paired_second_buster_effect(actor))continue;
        uint8_t priority=psx_mod_read_byte(actor+0x16);
        psx_mod_write_byte(actor+0x16,psx_mod_read_byte(PLAYER+0x16));
        render_actor(cpu,actor,base);psx_mod_write_byte(actor+0x16,priority);
    }
}
static void render(CPUState *cpu,uint32_t address) {
    (void)address;
    if(busy || projected || failed || !enrolled || !second.body[3] ||
       life.status[1]==MMX5_COOP_ABSENT || life.status[1]==MMX5_COOP_FALLEN)return;
    if(psx_mod_read_byte(PLAY)!=10 || !psx_mod_texture_banks_supported())return;
    busy=1;capture_player(&first);project_player(&second);
    uint32_t original=psx_mod_read_word(SCRATCH+0x100),mode=psx_mod_read_word(SCRATCH+0x104);
    uint32_t budget=psx_mod_read_word(SCRATCH+0x124);psx_mod_write_word(SCRATCH+0x124,0);
    uint32_t base=arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_BYTES;
    psx_mod_write_word(SCRATCH+0x100,base);psx_mod_write_word(SCRATCH+0x104,base+0x30000);
    int paired_car=paired_chaser_draw();
    if(paired_car)render_paired_chaser(cpu,1);
    render_paired_armor(cpu,1);
    render_actor(cpu,PLAYER,base);
    render_second_buster_effects(cpu,base);
    if(life.status[1]==MMX5_COOP_ALIVE) {
        /* Match native24698: X's three50h charge actors, then the three60h
         * dash trails in reverse order, all using this seat's retained art. */
        if(!second.body[2])for(unsigned i=0;i<3;++i) {
            uint32_t actor=GLOW+i*0x50u;
            if(psx_mod_read_byte(actor+3)) {
                uint8_t priority=psx_mod_read_byte(actor+0x16);
                if(life.status[0]==MMX5_COOP_ALIVE && first.body[3])
                    psx_mod_write_byte(actor+0x16,psx_mod_read_byte(PLAYER+0x16));
                render_actor(cpu,actor,base);psx_mod_counter_add("mmx5.coop.p2-charge-glow-drawn",1);
                psx_mod_write_byte(actor+0x16,priority);
            }
        }
        for(unsigned i=3;i>0;--i) {
            uint32_t actor=TRAILS+(i-1)*0x60u;
            if(psx_mod_read_byte(actor+3)) {
                /* Bike boost afterimages use the personal trail pool too.
                 * Their native bucket otherwise sorts these later quads
                 * over P1's completed bike/rider, despite the exhaust fix. */
                uint8_t priority=psx_mod_read_byte(actor+0x16);
                if(paired_car)psx_mod_write_byte(actor+0x16,psx_mod_read_byte(PLAYER+0x16));
                render_actor(cpu,actor,base);
                if(paired_car)psx_mod_write_byte(actor+0x16,priority);
                psx_mod_counter_add("mmx5.coop.p2-dash-trails-drawn",1);
            }
        }
    }
    for(unsigned i=0;i<32 && !failed;++i) {
        uint32_t actor=SHOTS+i*0x9Cu;if(!psx_mod_read_byte(actor+3))continue;
        uint8_t priority=psx_mod_read_byte(actor+0x16);
        int paired_projectile=life.status[0]==MMX5_COOP_ALIVE && life.status[1]==MMX5_COOP_ALIVE &&
            first.body[3] && second.body[3];
        /* Both X's private busters and Zero's class3A belong to P2's
         * complete draw group. Native projectile buckets otherwise place
         * them over P1 after that body has already been painted. Shared
         * world effects and every simulation priority remain untouched. */
        if(paired_projectile)psx_mod_write_byte(actor+0x16,psx_mod_read_byte(PLAYER+0x16));
        render_actor(cpu,actor,base);
        if(paired_projectile)psx_mod_write_byte(actor+0x16,priority);
    }
    /* Stage vehicle graphics remain the original shared stage atlas. They
     * are not character DAT frames and do not need a retained character bank. */
    if(!paired_car && car_context() && second.car[3])guest(cpu,0x80024C60,CAR,0);
    psx_mod_write_word(SCRATCH+0x100,original);psx_mod_write_word(SCRATCH+0x104,mode);
    psx_mod_write_word(SCRATCH+0x124,budget);
    project_player(&first);diagnostics();busy=0;
}
static int render_survivor(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!world_owner || !projected || render_call || !enrolled || failed)return 0;
    uint32_t actor=cpu->gpr[4];
    int glow_actor=actor>=GLOW && actor<GLOW+sizeof second.glow && !((actor-GLOW)%0x50u);
    int trail_actor=actor>=TRAILS && actor<TRAILS+sizeof second.trails && !((actor-TRAILS)%0x60u);
    if(actor!=PLAYER && !(actor>=SHOTS && actor<SHOTS+sizeof second.shots) && !glow_actor && !trail_actor)return 0;
    render_call=1;uint32_t original=psx_mod_read_word(SCRATCH+0x100),mode=psx_mod_read_word(SCRATCH+0x104);
    uint32_t base=arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_BYTES;
    capture(PLAYER,second.body,sizeof second.body);
    psx_mod_write_word(SCRATCH+0x100,world_packets);
    psx_mod_write_word(SCRATCH+0x104,base+0x30000+(world_packets-base)/5);
    render_actor(cpu,actor,base);world_packets=psx_mod_read_word(SCRATCH+0x100);
    if(glow_actor)psx_mod_counter_add("mmx5.coop.p2-charge-glow-drawn",1);
    if(trail_actor)psx_mod_counter_add("mmx5.coop.p2-dash-trails-drawn",1);
    int paused_primary=pause_owner==1 && psx_mod_read_byte(PLAY+1)==2 &&
        life.status[0]==MMX5_COOP_ALIVE;
    if(actor==PLAYER && first.body[3] && (paused_primary ||
       life.status[0]==MMX5_COOP_DYING || scene_passenger(0) || paired_victory)) {
        /* P2 owns the one world pass, but P1's native death/teleport or
         * paired victory pose retains its own sprite bank until it hides.
         * P2's pause fades also draw the original world. Retain P1's body
         * and frozen personal pools there; the native UI-only menu/Quit
         * paths never dispatch PLAYER and therefore append nothing. */
        PlayerContext paused_context;
        uint8_t body[0x158];capture(PLAYER,body,sizeof body);
        if(paused_primary) {capture_player(&paused_context);project_player(&first);}
        else project(PLAYER,first.body,sizeof body);
        uint32_t saved_resources[4];
        for(unsigned i=0;i<4;++i) {
            saved_resources[i]=psx_mod_read_word(SCRATCH+resource_offsets[i]);
            psx_mod_write_word(SCRATCH+resource_offsets[i],resources[i]);
        }
        uint32_t primary_base=primary_arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_BYTES;
        if(primary_packets<primary_base || primary_packets>=primary_base+40000)primary_packets=primary_base;
        psx_mod_write_word(SCRATCH+0x100,primary_packets);
        psx_mod_write_word(SCRATCH+0x104,primary_base+0x30000+(primary_packets-primary_base)/5);
        render_primary=1;render_actor(cpu,PLAYER,primary_base);
        if(paused_primary) {
            if(!first.body[2])for(unsigned i=0;i<3;++i)
                if(psx_mod_read_byte(GLOW+i*0x50u+3))render_actor(cpu,GLOW+i*0x50u,primary_base);
            for(unsigned i=3;i>0;--i)
                if(psx_mod_read_byte(TRAILS+(i-1)*0x60u+3))render_actor(cpu,TRAILS+(i-1)*0x60u,primary_base);
            for(unsigned i=0;i<32 && !failed;++i)
                if(psx_mod_read_byte(SHOTS+i*0x9Cu+3))render_actor(cpu,SHOTS+i*0x9Cu,primary_base);
        }
        render_primary=0;primary_packets=psx_mod_read_word(SCRATCH+0x100);
        if(paused_primary) {capture_player(&first);project_player(&paused_context);}
        else {capture(PLAYER,first.body,sizeof body);project(PLAYER,body,sizeof body);}
        for(unsigned i=0;i<4;++i)psx_mod_write_word(SCRATCH+resource_offsets[i],saved_resources[i]);
    }
    psx_mod_write_word(SCRATCH+0x100,original);psx_mod_write_word(SCRATCH+0x104,mode);
    render_call=0;return 1;
}
static int render_personal_effect(CPUState *cpu) {
    uint32_t actor=cpu->gpr[4];
    if(!enrolled || failed || psx_mod_read_byte(PLAY)!=10 ||
       ((busy || projected) && !(world_owner && projected)) ||
       !psx_mod_read_byte(actor))return 0;
    unsigned owner,landing_draw=0,death_draw=0,hit_draw=0,static_draw=0,effect_owner;
    if(private_death_actor(actor)) {owner=1;death_draw=1;}
    else if((effect_owner=(unsigned)private_static_effect(actor))) {
        owner=effect_owner-1;static_draw=!psx_mod_read_word(actor+0x38);
        unsigned kind=psx_mod_read_byte(actor+1);hit_draw=kind==0 || kind==3;
    }
    else if(actor>=0x80094398u && actor<0x80096D98u && !((actor-0x80094398u)%0x70u) &&
       (psx_mod_read_byte(actor+1)==5 || psx_mod_read_byte(actor+1)==0x11)) {
        uint32_t parent=psx_mod_read_word(actor+0x50);
        if(psx_mod_read_byte(actor+1)==5 &&
           (parent<SHOTS || parent>=SHOTS+sizeof second.shots || (parent-SHOTS)%0x9Cu))return 0;
        if(psx_mod_read_byte(actor+1)==0x11 && parent!=PLAYER)return 0;
        owner=personal_effect_owner[(actor-0x80094398u)/0x70u];
    } else {
        LandingEffect *landing=landing_effect(actor);
        if(!landing || !landing->owner || psx_mod_read_byte(actor+1)!=0x3F ||
           psx_mod_read_byte(actor+2))return 0;
        owner=landing->owner-1u;landing_draw=1;
    }
    uint32_t original=psx_mod_read_word(SCRATCH+0x100),mode=psx_mod_read_word(SCRATCH+0x104);
    uint32_t palette=psx_mod_read_word(SCRATCH+0x28);
    uint32_t base=primary_arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_BYTES;
    if(primary_packets<base || primary_packets>=base+40000)primary_packets=base;
    /* FX5/C-Sword11, tracked Zero landing3F and private death11 retain their
     * owner's assembly and position. Static death glyphs use retained native images,
     * not the current player's dynamic sprite directory. Draw each
     * once with that seat's decoded pixels/palette, without projecting or
     * advancing either player. The packet arena is shared storage, not an
     * ownership choice. P1's palette is saved while P2 owns the world pass. */
    uint32_t primary_resources[4];
    if(!owner && projected)for(unsigned i=0;i<4;++i) {
        primary_resources[i]=psx_mod_read_word(SCRATCH+resource_offsets[i]);
        psx_mod_write_word(SCRATCH+resource_offsets[i],resources[i]);
    }
    psx_mod_write_word(SCRATCH+0x100,primary_packets);
    psx_mod_write_word(SCRATCH+0x104,base+0x30000+(primary_packets-base)/5);
    render_call=1;render_primary=!owner;
    uint32_t before=primary_packets;
    render_actor(cpu,actor,base);primary_packets=psx_mod_read_word(SCRATCH+0x100);
    render_call=render_primary=0;
    psx_mod_write_word(SCRATCH+0x100,original);psx_mod_write_word(SCRATCH+0x104,mode);
    psx_mod_write_word(SCRATCH+0x28,palette);
    if(!owner && projected)for(unsigned i=0;i<4;++i)
        psx_mod_write_word(SCRATCH+resource_offsets[i],primary_resources[i]);
    if(primary_packets>before && death_draw)psx_mod_counter_add("mmx5.coop.p2-death-glyphs-drawn",1);
    else if(primary_packets>before && hit_draw)psx_mod_counter_add(owner?
        "mmx5.coop.p2-hit-effects-drawn":"mmx5.coop.p1-hit-effects-drawn",1);
    else if(primary_packets>before && static_draw)psx_mod_counter_add(owner?
        "mmx5.coop.p2-static-effects-drawn":"mmx5.coop.p1-static-effects-drawn",1);
    else if(primary_packets>before && (landing_draw || psx_mod_read_byte(actor+1)==5))psx_mod_counter_add(landing_draw?
        (owner?"mmx5.coop.p2-zero-landing-drawn":"mmx5.coop.p1-zero-landing-drawn"):
        (owner?"mmx5.coop.p2-cshot-effect-drawn":"mmx5.coop.p1-cshot-effect-drawn"),1);
    return 1;
}
static int dinorex_warning_draw(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];LandingEffect *effect=landing_effect(actor);
    if(!psx_mod_local_view_scope() || !split_views || !split_valid || !enrolled || failed ||
       !camera_emitter_stage() || !effect || !effect->owner || effect->owner>2 ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x14 ||
       psx_mod_read_byte(actor+2)<3 || psx_mod_read_byte(actor+2)>4 ||
       psx_mod_read_byte(actor+4)!=1 ||
       !camera_emitter_parent(psx_mod_read_word(actor+0x50)))return 0;
    unsigned owner=effect->owner-1u;
    int x,reference;
    if(owner==local_view_seat || !dinorex_fire_view_region(local_view_seat,&x) ||
       !dinorex_fire_view_region(owner,&reference))return 0;
    /* The native floor seal separates the lower corridor at minY2656.
     * Both rooms can see the fire cycle, but its camera-relative rubble
     * must not be copied through that ceiling into the upper approach. */
    if(((int16_t)camera_emitter_view(local_view_seat,0x2A)>=2656)!=
       ((int16_t)camera_emitter_view(owner,0x2A)>=2656)) {
        cpu->gpr[2]=0;return 1;
    }
    /* F3CD8/F3D84 create camera-relative warning debris for one shared
     * fire sequence. Present that same falling/animated warning in each
     * admitted lower view, as the fire sheet already is. Only this private
     * draw borrows coordinates; native RNG, terrain, lifetime and contacts
     * still run once at the original position. Upper-route art stays native. */
    uint32_t position=psx_mod_read_word(actor+8);
    int y=(int16_t)camera_emitter_view(local_view_seat,14)-
        (int16_t)camera_emitter_view(owner,14);
    uint32_t height=psx_mod_read_word(actor+12);
    psx_mod_write_word(actor+8,position+(uint32_t)((x-reference)*65536));
    psx_mod_write_word(actor+12,height+(uint32_t)(y*65536));
    render_call=1;
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    render_call=0;
    psx_mod_write_word(actor+8,position);psx_mod_write_word(actor+12,height);
    psx_mod_counter_add("mmx5.coop.local-view-dinorex-warning",1);
    cpu->gpr[2]=result;return 1;
}
static int pegasus_burst_draw(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(!psx_mod_local_view_scope() || !split_views || !split_valid || !enrolled || failed ||
       psx_mod_read_byte(PLAY+12)!=8 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=15u ||
       actor<0x80094398u || actor>=0x80096D98u || (actor-0x80094398u)%0x70u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=4 ||
       psx_mod_read_byte(actor+2) || psx_mod_read_word(actor+0x60)!=0x800EF5A0u)return 0;
    uint32_t origin=psx_mod_read_word(actor+0x6C);
    if(origin) {
        int x=(int16_t)origin-(int16_t)psx_mod_read_half(CAMERA+10);
        int y=(int16_t)(origin>>16)-(int16_t)psx_mod_read_half(CAMERA+14);
        int margin=psx_mod_widescreen_x_margin();
        /* Use the same conservative native sprite border as sprite_outside.
         * Only presentation is culled: the one original bomb, native damage,
         * animation, RNG and lifetime continue unchanged. */
        if(x < -144-margin || x > 464+margin || y < -144 || y > 384) {
            psx_mod_counter_add("mmx5.coop.local-view-distant-pegasus-bursts",1);
            cpu->gpr[2]=0;return 1;
        }
    }
    uint32_t x=psx_mod_read_word(actor+8),y=psx_mod_read_word(actor+12);
    psx_mod_write_word(actor+8,x+psx_mod_read_word(CAMERA+8)-psx_mod_read_word(actor+0x64));
    psx_mod_write_word(actor+12,y+psx_mod_read_word(CAMERA+12)-psx_mod_read_word(actor+0x68));
    render_call=1;uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);render_call=0;
    psx_mod_write_word(actor+8,x);psx_mod_write_word(actor+12,y);
    psx_mod_counter_add(local_view_seat?"mmx5.coop.local-view-p2-pegasus-bursts":
                                      "mmx5.coop.local-view-p1-pegasus-bursts",1);
    cpu->gpr[2]=result;return 1;
}
static int render_hook(CPUState *cpu,uint32_t address) {
    if(bindings_name_draw(cpu,address))return 1;
    if(armor_name_draw(cpu,address))return 1;
    if(render_call)return cull_sprite_draw(cpu,address);
    if(pegasus_burst_draw(cpu,address))return 1;
    if(dinorex_warning_draw(cpu,address))return 1;
    if(balance_draw(cpu,address))return 1;
    if(!busy && !projected && paired_second_buster_effect(cpu->gpr[4])) {
        cpu->gpr[2]=0;return 1;
    }
    if(render_personal_effect(cpu))return 1;
    if(render_survivor(cpu,address))return 1;
    uint32_t actor=cpu->gpr[4];
    int primary_glow=actor>=GLOW && actor<GLOW+sizeof first.glow && !((actor-GLOW)%0x50u);
    int primary_trail=actor>=TRAILS && actor<TRAILS+sizeof first.trails && !((actor-TRAILS)%0x60u);
    if(!busy && !projected && (actor==CAR || actor==CAR+0xB0u) &&
       (paired_armor_rider(0)==actor || paired_armor_rider(1)==actor)) {
        cpu->gpr[2]=0;return 1;
    }
    if((actor==CAR || chaser_exhaust_owner(actor)) && !busy && !projected && paired_chaser_draw()) {
        /* The native singleton bike draw is emitted with its rider below. */
        cpu->gpr[2]=0;return 1;
    }
    if(busy || projected || failed || !enrolled || psx_mod_read_byte(PLAY)!=10 ||
       (actor!=PLAYER && !(actor>=SHOTS && actor<SHOTS+sizeof second.shots) &&
        !primary_glow && !primary_trail))return cull_sprite_draw(cpu,address);
    /* Keep the body dispatch boundary even when P1 is offscreen: it also
     * emits P2. render_actor culls each body/shot against this view separately. */
    /* Native24698 paints X's armor after this body call. Emit the complete
     * partner first, so it cannot split P1's face/body from that armor in
     * their shared native bucket. Keep native actor priorities unchanged. */
    if(actor==PLAYER)render(cpu,address);
    uint32_t original=psx_mod_read_word(SCRATCH+0x100),mode=psx_mod_read_word(SCRATCH+0x104);
    uint32_t base=primary_arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_BYTES;
    if(primary_packets<base || primary_packets>=base+40000)primary_packets=base;
    capture(PLAYER,first.body,sizeof first.body);render_call=render_primary=1;
    psx_mod_write_word(SCRATCH+0x100,primary_packets);
    psx_mod_write_word(SCRATCH+0x104,base+0x30000+(primary_packets-base)/5);
    if(actor==PLAYER && paired_chaser_draw())render_paired_chaser(cpu,0);
    if(actor==PLAYER)render_paired_armor(cpu,0);
    render_actor(cpu,actor,base);primary_packets=psx_mod_read_word(SCRATCH+0x100);
    if(primary_glow)psx_mod_counter_add("mmx5.coop.p1-charge-glow-drawn",1);
    if(primary_trail)psx_mod_counter_add("mmx5.coop.p1-dash-trails-drawn",1);
    psx_mod_write_word(SCRATCH+0x100,original);psx_mod_write_word(SCRATCH+0x104,mode);
    render_call=render_primary=0;
    return 1;
}
static void private_partner_before_sort(CPUState *cpu,uint32_t address) {
    /* Native24698 omits the PLAYER draw when P1 is hidden. Append its living
     * partner before24B88 links the actor groups into the submitted draw list;
     * packets appended after272B4 returns cannot reach that list. */
    if(psx_mod_local_view_scope() && cpu->gpr[31]==0x80024B70u && !psx_mod_read_byte(PLAYER+3))
        render(cpu,address);
}
static int retained_tile_pixels(const CharacterAssets *character,uint16_t page,uint16_t uv,
                                unsigned reflection,unsigned directory,const uint16_t colors[16],
                                uint16_t pixels[256]) {
    uint8_t decoded[32768];unsigned tiles=0;
    if(directory && !ui_decode_compressed(character,directory,decoded,&tiles))return 0;
    unsigned bx=(page&15)*64,by=(page&16)*16;
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x) {
        unsigned u=((uv&255)+((reflection&1u)?15-x:x))&255;
        unsigned v=((uv>>8)+((reflection&2u)?15-y:y))&255;
        uint16_t word=character->ui_pixels[(by+v)*1024+bx+u/4];
        if(directory && bx+u/4>=320 && by+v>=64) {
            unsigned xword=bx+u/4-320,row=by+v-64,strip=row/16;
            if(strip*16<tiles) {
                unsigned count=tiles-strip*16,width=(count<16?count:16)*4;
                if(xword<width) {
                    unsigned offset=strip*2048+((row%16)*width+xword)*2;
                    word=(uint16_t)(decoded[offset]|decoded[offset+1]<<8);
                }
            }
        }
        pixels[y*16+x]=colors[(word>>((u&3)*4))&15];
    }
    return 1;
}
static uint16_t retained_tile_bank(uint16_t page,uint16_t clut,uint16_t uv,int personal,unsigned reflection,unsigned directory,unsigned character) {
    unsigned *used=psx_mod_local_view_scope()?&view_ui_bank_count:&ui_bank_count;
    UiBank *cache=psx_mod_local_view_scope()?view_ui_banks:ui_banks;
    if(page&0x180)return 0;
    uint16_t colors[16],pixels[256];unsigned cx=(clut&63)*16,cy=clut>>6;
    if(cx+16>1024 || cy>=512)return 0;
    size_t palette_offset=0;
    if(personal && !mmx5_coop_duel_palette_offset(clut,1,0x4000,&palette_offset))return 0;
    /* The native UI owns these CLUTs separately from character sprite colors.
     * The facade synchronizes GL before reading its current palette. */
    for(unsigned i=0;i<16;++i)colors[i]=personal?
        psx_mod_read_half((personal==2?psx_mod_read_word(SCRATCH+0x28):assets[character].palette)+
            (uint32_t)palette_offset+i*2u):
        clut==0x7841?assets[character].hud_colors[i]:gpu_vram_peek((int)(cx+i),(int)cy);
    if(personal)for(unsigned i=0;i<16;++i)
        colors[i]=retained_palette_color(colors[i],(unsigned)(palette_offset/32u));
    unsigned source=character_source(character)|(directory<<8);
    for(unsigned i=0;i<(*used);++i)
        if(cache[i].source==source && cache[i].page==page && cache[i].clut==clut &&
           cache[i].uv==uv && cache[i].reflection==reflection &&
           !memcmp(cache[i].colors,colors,sizeof colors))return cache[i].id;
    unsigned first_id=psx_mod_local_view_scope()?0xB000u:0x7000u;
    unsigned slot=mmx5_coop_texture_cache_slot(*used,(uint16_t)first_id,
        psx_mod_read_word(PLAY+0x1B4),arena,primary_arena,hud_arena,psx_mod_read_word);
    if(slot>=4096) {psx_mod_counter_add("mmx5.coop.ui-cache-pinned",1);return 0;}
    if(!retained_tile_pixels(&assets[character],page,uv,reflection,directory,colors,pixels))return 0;
    uint16_t id=(uint16_t)(first_id+slot);unsigned replacing=slot<*used;
    if(replacing && !psx_mod_forget_texture_bank_range(id,1))return 0;
    if(!psx_mod_define_texture_bank(id,16,16,pixels)) {
        if(replacing)cache[slot].source=~0u;
        return 0;
    }
    if(!replacing)++(*used);else psx_mod_counter_add("mmx5.coop.ui-cache-reuses",1);
    UiBank *bank=&cache[slot];bank->page=page;bank->clut=clut;bank->uv=uv;
    bank->id=id;bank->source=source;bank->reflection=reflection;
    memcpy(bank->colors,colors,sizeof colors);return id;
}
static void native_hud_tiles(uint32_t begin,uint32_t mode,unsigned character) {
    if(!psx_mod_local_view_scope())return;
    uint32_t end=psx_mod_read_word(SCRATCH+0x108);
    if(end<begin || end-begin>0x1000 || (end-begin)%16)return;
    /* Personal views and the duel retain the original HUD packet ordering
     * and anchors. Supply the selected hero's sampled UI tiles inside this
     * view transaction, without persisting P2 art into world VRAM. The bar
     * fill alone is untextured; the frame and badge need these native tiles. */
    for(uint32_t at=begin;at<end;at+=16,mode+=8) {
        unsigned page=psx_mod_read_word(mode+4)&0xFFFFu;
        uint32_t uv=psx_mod_read_word(at+12);
        unsigned u=uv&255u,v=(uv>>8)&255u;
        if((page&0x180u) || (u&3u) || u>240 || v>240)continue;
        unsigned x=(page&15u)*64u+u/4u,y=(page&16u)*16u+v;
        uint16_t tile[64];
        for(unsigned row=0;row<16;++row)
            memcpy(tile+row*4,assets[character].ui_pixels+(y+row)*1024u+x,8);
        gr_vram_transfer_in(x,y,4,16,tile);
        if((uv>>16)==0x7841u)
            gr_vram_transfer_in(16,481,16,1,assets[character].hud_colors);
    }
}
static int hud_icon(CPUState *cpu,uint32_t address) {
    /* Normal story-boss frame and its two level glyphs form one HUD group.
     * Shift every emitted fragment, including frame pieces left of its anchor. */
    if(duel_scope() && hud_call && !hud_icon_call &&
       ((cpu->gpr[31]==0x80025A48u && cpu->gpr[4]==7) ||
        (cpu->gpr[31]==0x800259A4u && cpu->gpr[4]==14) ||
        (cpu->gpr[31]==0x800259C4u && cpu->gpr[4]==15))) {
        hud_icon_call=1;uint32_t begin=psx_mod_read_word(SCRATCH+0x108);
        uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        uint32_t end=psx_mod_read_word(SCRATCH+0x108);
        if(end>=begin && end-begin<=0x1000 && !((end-begin)%16))
            for(uint32_t at=begin;at<end;at+=16)
                psx_mod_write_half(at+10,(uint16_t)(psx_mod_read_half(at+10)+29));
        hud_icon_call=0;cpu->gpr[2]=result;return 1;
    }
    if(!psx_mod_local_view_scope() && hud_seat==1 && (cpu->gpr[4]==3 || cpu->gpr[4]==4)) {cpu->gpr[2]=0;return 1;}
    if(psx_mod_local_view_scope())return 0;
    if(hud_icon_call || !hud_call || hud_seat || hud_side_by_side || cpu->gpr[4]!=10)return 0;
    /* Native virus/radar block occupies the lower left in Maverick stages.
     * Move its existing sprites together so the stacked P2 meter stays clear. */
    hud_icon_call=1;uint32_t start=psx_mod_read_word(SCRATCH+0x108);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    uint32_t end=psx_mod_read_word(SCRATCH+0x108);
    if(end>=start && end-start<=0x1000 && !((end-start)%16))
        for(uint32_t at=start;at<end;at+=16) {
            psx_mod_write_half(at+8,(uint16_t)(psx_mod_read_half(at+8)+48));
            /* The shifted composite can cross the automatic HUD anchor
             * threshold. Keep every Normal/Virus glyph on the same left
             * anchor instead of separating its trailing tiles at wide aspect. */
            psx_mod_anchor_hud_primitive(at,-1);
        }
    hud_icon_call=0;cpu->gpr[2]=result;return 1;
}
static void hud_packets(uint32_t base,unsigned character,int duel_right,unsigned capacity) {
    uint32_t sprites=psx_mod_read_word(SCRATCH+0x108),modes=psx_mod_read_word(SCRATCH+0x10C);
    uint32_t bars=psx_mod_read_word(SCRATCH+0x110);
    if(sprites<base || sprites>base+0x1000 || (sprites-base)%16 ||
       modes<base+0x1000 || modes>base+0x2000 ||
       (modes-base-0x1000)!=(sprites-base)/2 ||
       bars<base+0x2000 || bars>base+0x3000 || (bars-base-0x2000)%36) {
        psx_mod_write_word(diagnostic+0xF00,2);psx_mod_write_word(diagnostic+0xF04,sprites-base);
        psx_mod_write_word(diagnostic+0xF08,modes-base);psx_mod_write_word(diagnostic+0xF0C,bars-base);
        failed=1;return;
    }
    uint32_t expanded=base+0x3000,mode=base+0x1000;
    for(uint32_t at=base;at<sprites;at+=16,expanded+=80,mode+=8) {
        uint32_t link=psx_mod_read_word(at),uv=psx_mod_read_word(at+12);
        uint16_t page=(uint16_t)psx_mod_read_word(mode+4),bank=retained_tile_bank(page,(uint16_t)(uv>>16),(uint16_t)uv,0,0,0,character);
        if(!bank || expanded+80>base+capacity) {
            psx_mod_write_word(diagnostic+0xF00,3);psx_mod_write_word(diagnostic+0xF04,page);
            psx_mod_write_word(diagnostic+0xF08,uv);psx_mod_write_word(diagnostic+0xF0C,expanded-base);
            failed=1;return;
        }
        int offset_x=duel_right?272:!duel_hud_active() && !psx_mod_local_view_scope() && hud_side_by_side?56:0;
        int offset_y=!duel_hud_active() && !psx_mod_local_view_scope() && !hud_side_by_side?SECOND_HUD_Y:0;
        int x=(int16_t)psx_mod_read_half(at+8)+offset_x,y=(int16_t)psx_mod_read_half(at+10)+offset_y;
        uint32_t q[10]={link,0x2C000000u|hud_fade*0x010101u,0,0,0,0,0,0,0,0};
        for(unsigned i=0;i<4;++i) {
            q[2+i*2]=(uint16_t)(x+(i&1)*16)|((uint32_t)(uint16_t)(y+(i>>1)*16)<<16);
            q[3+i*2]=(i&1)*16|((i>>1)*16<<8);
        }
        triangle(expanded,expanded+40,q,0,1,2,bank);triangle(expanded+40,link,q,2,1,3,bank);
        psx_mod_anchor_hud_primitive(expanded,duel_right?1:-1);psx_mod_anchor_hud_primitive(expanded+40,duel_right?1:-1);
        psx_mod_write_word(at,expanded&0xFFFFFFu);
    }
    for(uint32_t at=base+0x2000;at<bars;at+=36) {
        int offset_x=duel_right?272:!duel_hud_active() && !psx_mod_local_view_scope() && hud_side_by_side?56:0;
        int offset_y=!duel_hud_active() && !psx_mod_local_view_scope() && !hud_side_by_side?SECOND_HUD_Y:0;
        for(unsigned xy=8;xy<=32;xy+=8) {
            psx_mod_write_half(at+xy,(uint16_t)(psx_mod_read_half(at+xy)+offset_x));
            psx_mod_write_half(at+xy+2,(uint16_t)(psx_mod_read_half(at+xy+2)+offset_y));
        }
        for(unsigned c=4;c<=28;c+=8) {
            uint32_t color=psx_mod_read_word(at+c),faded=color&0xFF000000u;
            for(unsigned channel=0;channel<24;channel+=8)faded|=(((color>>channel)&255)*hud_fade/128)<<channel;
            psx_mod_write_word(at+c,faded);
        }
        psx_mod_anchor_hud_primitive(at,duel_right?1:-1);
    }
    psx_mod_counter_add("mmx5.coop.hud-secondary-sprites",(sprites-base)/16u);
    psx_mod_counter_add("mmx5.coop.hud-secondary-bars",(bars-base-0x2000u)/36u);
}
/* Draw each duelist with the same native personal life/weapon HUD used in
 * normal co-op. Retained UI banks keep both hero logos and ammo colors valid
 * even when a later native upload belongs to the other participant. */
static uint32_t draw_duel_hud(CPUState *cpu) {
    uint8_t body[sizeof first.body];capture(PLAYER,body,sizeof body);
    uint8_t campaign=psx_mod_read_byte(PLAY+0x44);
    uint8_t maxima[2]={psx_mod_read_byte(PLAY+0x47),psx_mod_read_byte(PLAY+0x48)};
    uint32_t previous[6];for(unsigned i=0;i<6;++i)previous[i]=psx_mod_read_word(SCRATCH+resource_offsets[i]);
    uint32_t pools[3];for(unsigned i=0;i<3;++i)pools[i]=psx_mod_read_word(SCRATCH+0x108+i*4);
    int filling=!story_duel.intro_complete;
    unsigned fill=DUEL_HP;
    if(filling) {
        /* Both personal meters follow the original NPC's one fill clock.
         * Only this temporary HUD body sees the projected value: neither
         * participant's health, injury state nor native readiness changes. */
        unsigned maximum=psx_mod_read_byte(PLAY+0xA2);
        unsigned current=psx_mod_read_byte(story_duel.opponent+0x5C)&127u;
        fill=maximum?current*DUEL_HP/maximum:0;
        if(fill>DUEL_HP)fill=DUEL_HP;
    }
    hud_call=1;hud_fade=128;
    for(unsigned side=0;side<2 && !failed;++side) {
        unsigned seat=story_duel.entrant^side,hero=story_duel.character[seat];
        project(PLAYER,seat?second.body:first.body,sizeof body);
        if(filling) {
            psx_mod_write_byte(PLAYER+0x5C,(uint8_t)fill);
            psx_mod_write_byte(PLAYER+0x5D,(uint8_t)fill);
        }
        psx_mod_write_byte(PLAY+0x44,(uint8_t)hero);
        psx_mod_write_byte(PLAY+0x47,DUEL_HP);psx_mod_write_byte(PLAY+0x48,DUEL_HP);
        psx_mod_write_word(SCRATCH+0x1C,assets[hero].assembly);
        psx_mod_write_word(SCRATCH+0x3C,assets[hero].menu_layout);
        /* 0x4010 is divisible by both native16-byte sprite and40-byte
         * retained-triangle strides, so cache pinning sees the right HUD. */
        uint32_t base=hud_arena+(psx_mod_read_word(SCRATCH)&1u)*0x8000+side*0x4010;
        for(unsigned i=0;i<3;++i)psx_mod_write_word(SCRATCH+0x108+i*4,base+i*0x1000);
        hud_seat=seat;
        guest(cpu,0x800262D8u,PLAYER,0);
        guest(cpu,hero?0x80025D94u:0x80025A74u,PLAYER,0);
        hud_packets(base,hero,(int)side,side?0x3FF0:0x4000);
    }
    project(PLAYER,body,sizeof body);psx_mod_write_byte(PLAY+0x44,campaign);
    psx_mod_write_byte(PLAY+0x47,maxima[0]);psx_mod_write_byte(PLAY+0x48,maxima[1]);
    for(unsigned i=0;i<6;++i)psx_mod_write_word(SCRATCH+resource_offsets[i],previous[i]);
    for(unsigned i=0;i<3;++i)psx_mod_write_word(SCRATCH+0x108+i*4,pools[i]);
    hud_call=hud_seat=0;return 0;
}
static int scene_return_present(void) {
    /* A capsule/shared camera can remain owned until the companion's beam
     * finishes, after native gameplay has already unlocked. Keep that camera
     * and each pane's own HUD during this short recovery. */
    return scene_owner && !paired_victory && scene_phase>=3 && scene_phase<=5 &&
        !psx_mod_read_byte(PLAY+0x10) && !psx_mod_read_byte(PLAY+0x1C);
}
static int shared_scene_pair_present(void) {
    /* Shared introductions still have two personal HUD owners. Redraw the
     * same native scene in both panes instead of copying its two-meter OT.
     * The story duel retains its separate, deliberately paired meter layout. */
    return local_split && scene_owner && !duel_scope() &&
        psx_mod_read_byte(PLAY)==10 && !psx_mod_read_byte(PLAY+1) &&
        !psx_mod_read_byte(PLAY+0xF);
}
static int paired_stage_arrival(void) {
    if(scene_owner || psx_mod_read_byte(PLAYER+4)!=1)return 0;
    /* Ride Chaser keeps native1C throughout driving, rather than releasing
     * it after teleport-in. The room remains shared; its HUDs are personal. */
    if(car_context())return 1;
    if(join_phase>=3 && life.status[1]==MMX5_COOP_JOINING)return 1;
    /* The partner can finish resolving before P1's native action0 releases
     * the stage-entry hold. Keep private panes through that final interval;
     * otherwise one shared two-meter frame interrupts the arriving pair. */
    return life.status[1]==MMX5_COOP_ALIVE && !psx_mod_read_byte(PLAYER+5) &&
        psx_mod_read_byte(PLAY+0x1C) && !psx_mod_read_byte(PLAY+0x10);
}
static int draw_hud(CPUState *cpu,uint32_t address) {
    if(hud_call || !enrolled || failed || psx_mod_read_byte(PLAY)!=10)return 0;
    /* The flat fallback is duplicated into both panes. During paired arrival
     * or a shared scene it must not contain two-player meters; each private draw
     * emits its own native HUD, at its final position, from the first frame. */
    if(local_split && !psx_mod_local_view_scope() &&
       (paired_stage_arrival() || shared_scene_pair_present())) {
        psx_mod_counter_add("mmx5.coop.spawn-shared-hud-suppressed",1);
        cpu->gpr[2]=0;return 1;
    }
    /* PLAY24 is the original boss-meter admission. Reveal both personal
     * meters at that same point, including its empty first frame. They use
     * the native shell's fill fraction without exposing its48/96HP scale. */
    if(duel_scope() && !story_duel.intro_complete &&
       (!story_duel.opponent || !psx_mod_read_byte(PLAY+0x24))) {
        cpu->gpr[2]=0;return 1;
    }
    if(duel_scope()) {cpu->gpr[2]=draw_duel_hud(cpu);return 1;}
    if(split_views && psx_mod_local_view_scope()) {
        hud_call=1;uint32_t result;
        unsigned seat=duel_hud_active()?story_duel.entrant:local_hud_seat;
        if(!seat && !(duel_scope() && story_duel.phase==4)) {
            uint32_t sprites=psx_mod_read_word(SCRATCH+0x108),modes=psx_mod_read_word(SCRATCH+0x10C);
            result=duel_native_hud(cpu,address,cpu->gpr[4],cpu->gpr[5]);
            /* A previous campaign or the partner's uploads can own shared
             * UI VRAM. P1 needs its retained native tiles just like P2. */
            native_hud_tiles(sprites,modes,first.body[2]);
        } else {
            uint8_t current[sizeof second.body];capture(PLAYER,current,sizeof current);
            uint32_t assembly_now=psx_mod_read_word(SCRATCH+0x1C),menu_now=psx_mod_read_word(SCRATCH+0x3C);
            uint8_t character=psx_mod_read_byte(PLAY+0x44);
            unsigned hero=seat?counterpart:story_duel.character[0];
            project(PLAYER,seat?second.body:first.body,sizeof second.body);
            psx_mod_write_byte(PLAY+0x44,(uint8_t)hero);
            psx_mod_write_word(SCRATCH+0x1C,assets[hero].assembly);
            psx_mod_write_word(SCRATCH+0x3C,assets[hero].menu_layout);
            uint32_t sprites=psx_mod_read_word(SCRATCH+0x108),modes=psx_mod_read_word(SCRATCH+0x10C);
            /* A private view already isolates VRAM and native packet pools.
             * Keep the original HUD sprites, their sorting and frame/badge;
             * the shared-screen retained conversion is unnecessary here. */
            hud_fade=128;hud_seat=seat;result=duel_native_hud(cpu,address,cpu->gpr[4],cpu->gpr[5]);
            hud_seat=0;native_hud_tiles(sprites,modes,hero);
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
    uint32_t result=duel_native_hud(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    if(world_owner)capture(PLAYER,first.body,sizeof first.body);
    else capture(PLAYER,current,sizeof current);
    if(psx_mod_read_byte(PLAY+0x1F) && hud_fade &&
       (!duel_hud_active() || story_duel.entrant==1)) {
        project(PLAYER,world_owner?current:second.body,sizeof second.body);
        psx_mod_write_word(SCRATCH+0x1C,assets[counterpart].assembly);
        psx_mod_write_word(SCRATCH+0x3C,assets[counterpart].menu_layout);
        uint32_t pools[3];for(unsigned i=0;i<3;++i)pools[i]=psx_mod_read_word(SCRATCH+0x108+i*4);
        uint32_t base=hud_arena+(psx_mod_read_word(SCRATCH)&1u)*0x8000;
        for(unsigned i=0;i<3;++i)psx_mod_write_word(SCRATCH+0x108+i*4,base+i*0x1000);
        hud_seat=1;guest(cpu,0x800262D8,PLAYER,0);
        guest(cpu,counterpart?0x80025D94:0x80025A74,PLAYER,0);hud_seat=0;hud_packets(base,counterpart,0,0x8000);
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
    unsigned projected,busy,world_owner,render_call,render_primary,hud_call,hud_seat,hud_fade,hud_icon_call,upload_call,failed;
    Mmx5WidescreenViewContext widescreen;
    uint8_t tank_contents[sizeof private_tank_contents];
    unsigned etanks_initialized,menu_transient[4];
} LocalViewContext;
static void local_context_save(LocalViewContext *context) {
    *context=(LocalViewContext){first,second,{0},{0},{0},{0},world_packets,primary_packets,
        projected,busy,world_owner,render_call,render_primary,hud_call,hud_seat,hud_fade,hud_icon_call,upload_call,failed,{0},{0},0,{0}};
    memcpy(context->play,saved_play,sizeof saved_play);memcpy(context->camera,saved_camera,sizeof saved_camera);
    memcpy(context->car_owners,car_source_owner,sizeof car_source_owner);memcpy(context->resources,resources,sizeof resources);
    memcpy(context->tank_contents,private_tank_contents,sizeof private_tank_contents);
    context->etanks_initialized=etanks_initialized;
    context->menu_transient[0]=personal_menu_call;context->menu_transient[1]=personal_menu_seat;
    context->menu_transient[2]=personal_rendering;context->menu_transient[3]=personal_shared_call;
    mmx5_widescreen_view_save(&context->widescreen);
}
static void local_context_restore(const LocalViewContext *context) {
    first=context->first;second=context->second;
    memcpy(saved_play,context->play,sizeof saved_play);memcpy(saved_camera,context->camera,sizeof saved_camera);
    memcpy(car_source_owner,context->car_owners,sizeof car_source_owner);memcpy(resources,context->resources,sizeof resources);
    memcpy(private_tank_contents,context->tank_contents,sizeof private_tank_contents);
    etanks_initialized=context->etanks_initialized;
    world_packets=context->world_packets;primary_packets=context->primary_packets;
    projected=context->projected;busy=context->busy;world_owner=context->world_owner;
    render_call=context->render_call;render_primary=context->render_primary;hud_call=context->hud_call;
    hud_seat=context->hud_seat;hud_fade=context->hud_fade;hud_icon_call=context->hud_icon_call;
    upload_call=context->upload_call;failed=context->failed;
    personal_menu_call=context->menu_transient[0];personal_menu_seat=context->menu_transient[1];
    personal_rendering=context->menu_transient[2];personal_shared_call=context->menu_transient[3];
    mmx5_widescreen_view_restore(&context->widescreen);
}
static int terminal_death_present(void) {
    /* Native fatal entry locks the singleton stage before its death effect
     * and retry fade finish. Those flags stop controls, not saved views. */
    return life.wipe && !scene_owner && enrolled && !failed && !duel_scope() &&
        psx_mod_read_byte(PLAY)==10 &&
        (life.status[0]==MMX5_COOP_DYING || life.status[0]==MMX5_COOP_FALLEN) &&
        (life.status[1]==MMX5_COOP_DYING || life.status[1]==MMX5_COOP_FALLEN);
}
static unsigned terminal_death_shade(void) {
    if(!terminal_death_present())return 0;
    if(!psx_mod_read_byte(PERSONAL_FADE))return psx_mod_read_byte(PLAY+1)==1?255:0;
    /* Replay the already emitted original1303C TILE, without advancing the
     * single fade task for an additional view. Preserve its black endpoint. */
    return psx_mod_read_byte(0x800C9568u+(psx_mod_read_word(SCRATCH)&1u)*16u+4);
}
static unsigned chaser_arrival_shade(uint32_t buffer) {
    /* BIN7's arrival uses native layer2 mode10: 29F70 eases its initial
     * origin while 1303C covers the frame. Retain that fade in the split
     * redraw, only for this original road overlay and arrival phase. */
    if(psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+1) ||
       psx_mod_read_byte(PLAY+12)!=5 || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_byte(PLAY+0x1D) || psx_mod_read_word(0x800EE970u)!=11 ||
       psx_mod_read_word(0x800EF05Cu)!=0x8202009Bu ||
       psx_mod_read_word(0x800EF06Cu)!=0x94A3000Au ||
       psx_mod_read_byte(CAR+4)!=1 || psx_mod_read_byte(CAR+5) ||
       psx_mod_read_byte(CAMERA+0xAC)!=10)return 0;
    unsigned parity=psx_mod_read_word(SCRATCH)&1u;
    uint32_t tile=0x800C9568u+parity*16u,mode=0x800D51E8u+parity*8u;
    uint32_t color=psx_mod_read_word(tile+4),shade=color&255u;
    if(color!=(0x62000000u|shade*0x010101u) ||
       psx_mod_read_word(tile+8) || psx_mod_read_word(tile+12)!=0x00F00140u ||
       psx_mod_read_word(mode+4)!=0xE1000045u ||
       (psx_mod_read_word(mode)&0xFFFFFFu)!=(tile&0xFFFFFFu))return 0;
    /* Original1303C linked this frame's subtractive TILE at OT bucket78
     * before decrementing its fade. The live fade byte can already be zero
     * on its final dim frame; a retained packet alone can instead be stale.
     * Require the original current OT to own this exact mode/TILE pair.
     * Read before this private draw clears that OT; never step the task. */
    uint32_t at=psx_mod_read_word(buffer+0x78)&0xFFFFFFu;
    for(unsigned i=0;i<128 && at<=0x1FFFFCu && !(at&3u);++i) {
        if(at==(mode&0xFFFFFFu))return shade;
        at=psx_mod_read_word(0x80000000u|at)&0xFFFFFFu;
    }
    return 0;
}
static void intro_boss_draw_layer(const uint8_t canonical[0x54]) {
    /* BIN0 paints Sigma through layer1. F7EF0/F9B20 publish its visibility
     * and animated atlas origin after the ordinary camera updates. A private
     * camera's mode2 update instead leaves the stage's spatial origin here.
     * Project only the authored head fields inside this draw transaction;
     * retain the view's foreground camera, histories, bounds and simulation. */
    if(!psx_mod_local_view_scope() || !enrolled || failed ||
       psx_mod_read_byte(PLAY)!=10 || psx_mod_read_byte(PLAY+12) ||
       psx_mod_read_byte(PLAY+13) || psx_mod_read_word(0x800EE970u)!=4 ||
       psx_mod_read_word(0x800F7F50u)!=0xA0600057u ||
       psx_mod_read_word(0x800F9B34u)!=0xA462005Eu ||
       psx_mod_read_word(0x800F9B48u)!=0xA4620062u ||
       psx_mod_read_word(0x800F8534u)!=0xA0400098u ||
       psx_mod_read_word(0x800F8538u)!=0xA04000A6u)return;
    uint32_t actor=psx_mod_read_word(PLAY+0x20);
    if(actor<0x80092090u || actor>=0x80093DD0u || (actor-0x80092090u)%0x9Cu ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x31 ||
       psx_mod_read_byte(actor+2) || !psx_mod_read_byte(actor+0x90))return;
    psx_mod_write_byte(CAMERA+0x57,canonical[3]);
    psx_mod_write_half(CAMERA+0x5E,(uint16_t)(canonical[10]|canonical[11]<<8));
    psx_mod_write_half(CAMERA+0x62,(uint16_t)(canonical[14]|canonical[15]<<8));
    psx_mod_write_byte(CAMERA+0x98,canonical[0x44]);
    psx_mod_write_byte(CAMERA+0xA6,canonical[0x52]);
    psx_mod_counter_add("mmx5.coop.local-view-intro-boss-layer",1);
}
static void dinorex_fire_draw_layer(const uint8_t canonical[0xFC]) {
    /* Native AI and the committed world draw own one fire animation. Replay
     * its current layer, including the original hide command, into each view.
     * Admitting-body death changes targeting, never the sheet's lifetime. */
    if(!psx_mod_local_view_scope() || !split_views || !split_valid || !enrolled || failed ||
       !dinorex_fire_content() ||
       psx_mod_read_word(0x800F31A0u)!=0xA0A200A1u ||
       psx_mod_read_word(0x800F35D4u)!=0xA0E00057u)return;
    int left=(int16_t)psx_mod_read_half(0x800F5960u);
    int right=(int16_t)psx_mod_read_half(0x800F5962u);
    int x=(int16_t)psx_mod_read_half(CAMERA+10);
    project(CAMERA+0x54,canonical+0x54,0x54);
    int view_x;
    int visible_region=dinorex_fire_view_region(local_view_seat,&view_x);
    if(visible_region) {
        int reference_x=left+64;
        int delta=view_x-reference_x;
        left+=delta;right+=delta;
        /* Sweep's native parent0 adds the pane's foreground X. Retain the
         * same relative front and damage band in that admitted lower view.
         * Hold/retraction use parent-1 and keep their one wrapped page clock. */
        if(!psx_mod_read_byte(CAMERA+0xA6))
            psx_mod_write_half(CAMERA+0x5E,(uint16_t)(psx_mod_read_half(CAMERA+0x5E)-delta));
    }
    /* Native2A4E4 calls mode9 only while the layer's follow flag is set.
     * The fire program disables it to own an absolute scrolling origin;
     * adding foreground Y in that state moves the sheet off-screen. */
    if(psx_mod_read_byte(CAMERA+0x98) && psx_mod_read_byte(CAMERA+0x58)==9)
        psx_mod_write_half(CAMERA+0x62,psx_mod_read_half(CAMERA+14)+
            psx_mod_read_half(CAMERA+0x96));
    /* The original CP2 approach already admits the burst at cameraY2416.
     * Its upper ledge and lower corridor share the image/contact mapping;
     * F29B4 still owns the native terrain, cover and fatal predicates. */
    if(!visible_region || (int16_t)psx_mod_read_half(CAMERA+0x1A)<2192 ||
       (int16_t)psx_mod_read_half(CAMERA+0x28)<2416 ||
       right<=left || x>=right || x+320<=left)
        psx_mod_write_byte(CAMERA+0x57,0);
    psx_mod_counter_add("mmx5.coop.local-view-dinorex-fire-layer",1);
}
static void grizzly_countdown_view(void) {
    if(!psx_mod_local_view_scope() || !grizzly_stage() ||
       psx_mod_read_word(0x800FD22Cu)!=0x800F63E4u ||
       psx_mod_read_word(0x800F63E4u)!=0x27BDFFE0u)return;
    int x=(int16_t)(both_body_word(local_view_seat,8)>>16);
    for(uint32_t actor=0x80092090u;actor<0x80093DD0u;actor+=0x9Cu) {
        unsigned subtype=psx_mod_read_byte(actor+2);
        if(!psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x10 ||
           subtype<0x20 || subtype>0x25 || psx_mod_read_byte(actor+4)!=4 ||
           psx_mod_read_byte(actor+0x14)!=255)continue;
        uint32_t bounds=0x800FD204u+(subtype&15u)*4u;
        /* F63E4 advances the one native countdown, then selects its HUD
         * visibility from PLAYER.x. Screen-relative children are nearest
         * the trailing hero, so that shared flag can hide a leader's timer.
         * Reapply only its inclusive native interval for this pane. The
         * local draw transaction restores the actor; no timer ticks here. */
        psx_mod_write_byte(actor+3,(uint8_t)(
            x>=(int16_t)psx_mod_read_half(bounds) &&
            x<=(int16_t)psx_mod_read_half(bounds+2)));
    }
}
static int draw_local_view(CPUState *cpu,void *user,uint32_t alpha) {
    (void)alpha;uint32_t buffer=*(uint32_t *)user;
    int menu=personal_menu_present();
    unsigned arrival_shade=menu?0:chaser_arrival_shade(buffer);
    if(!menu) {
        uint8_t canonical[0xFC];capture(CAMERA,canonical,sizeof canonical);
        /* A door/capsule owns the canonical scripted camera until recovery.
         * Keep that view during private HUD draws without advancing its script. */
        if(!shared_scene_pair_present() || scene_return_present())
            project(CAMERA,split_cameras[local_view_seat],sizeof saved_camera);
        intro_boss_draw_layer(canonical+0x54);
        dinorex_fire_draw_layer(canonical);
        split_scenery_origins(cpu);
        grizzly_countdown_view();
    }
    if(!menu && !mmx5_widescreen_project_region_atlas())return 0;
    gpu_set_gp0_source(0);
    gpu_write_gp0(0x02000000u|psx_mod_read_byte(buffer+0x2D)|
        (uint32_t)psx_mod_read_byte(buffer+0x2E)<<8|(uint32_t)psx_mod_read_byte(buffer+0x2F)<<16);
    gpu_write_gp0(psx_mod_read_half(buffer+0x14)|(uint32_t)psx_mod_read_half(buffer+0x16)<<16);
    gpu_write_gp0(psx_mod_read_half(buffer+0x18)|(uint32_t)psx_mod_read_half(buffer+0x1A)<<16);
    guest(cpu,0x80068B20,buffer+0x70,16);
    if(menu) {if(!personal_menu_draw(cpu,local_view_seat))return 0;}
    else guest(cpu,0x800272B4,0,0);
    if(failed || projected || busy || render_call || hud_call)return 0;
    guest(cpu,0x8006A314,buffer+0xAC,0);
    personal_menu_draw_shade(personal_active?personal_menu_shade():
        terminal_death_present()?terminal_death_shade():arrival_shade);
    return !failed;
}
static int draw_local_pair(CPUState *cpu,void *user,uint32_t seat) {
    LocalViewContext context;local_context_save(&context);
    local_view_seat=personal_menu_present()?seat:split_view_seat(seat);
    /* Camera ownership may gather or spectate the living partner; a pane's
     * native health/resource display still belongs to its assigned player. */
    local_hud_seat=seat;
    int ok=draw_local_view(cpu,user,0);
    /* Each pane starts with the same host context as well as the framework's
     * CPU/RAM/VRAM checkpoint. A private draw never advances world logic. */
    local_context_restore(&context);
    return ok;
}
static int intro_reveal_full_white(void) {
    if(!enrolled || failed || psx_mod_read_byte(PLAY)!=10 ||
       psx_mod_read_byte(PLAY+12) || psx_mod_read_byte(PLAY+13) ||
       psx_mod_read_word(0x800EE970u)!=4)return 0;
    uint32_t actor=psx_mod_read_word(PLAY+0x20);
    if(actor<0x80092090u || actor>=0x80093DD0u || (actor-0x80092090u)%0x9Cu ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=0x31 ||
       psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+5)!=2)return 0;
    const uint32_t channels[]={0x800D50DCu,0x8009A42Cu,0x80093E66u};
    const unsigned full[]={31,992,31744};
    for(unsigned i=0;i<3;++i)
        if(psx_mod_read_byte(channels[i]) || psx_mod_read_half(channels[i]+2)<full[i])return 0;
    return 1;
}
static unsigned apply_local_split_aspect(void) {
    return (unsigned)psx_mod_set_fixed_display_aspect(local_split_wide?16u:4u,local_split_wide?9u:3u);
}
static void enable_local_pair_presentation(void) {
    if(local_split_wide)psx_mod_set_local_pair_wide_presentation();
    else psx_mod_set_local_pair_presentation(1);
}
static int present_local_view(CPUState *cpu,uint32_t address) {
    uint32_t buffer=psx_mod_read_word(0x8009E794u);
    if(!local_view_call && !projected && !busy && !psx_mod_local_view_scope() &&
       cpu->gpr[31]==0x80011E50u && cpu->gpr[4]==buffer+0xAC && intro_reveal_full_white()) {
        /* The native palette has reached its fully white reveal endpoint.
         * Transparent scenery and the retained HUD otherwise leave holes
         * in that white frame. Complete the same native viewport after its
         * OT, including the shared-scene path that skips private redraws.
         * The original RGB program alone owns the duration and release. */
        local_view_call=1;
        uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        guest(cpu,0x80068634,0,0);gpu_set_gp0_source(0);
        gpu_write_gp0(0x02FFFFFFu);
        gpu_write_gp0(psx_mod_read_half(buffer+0x14)|(uint32_t)psx_mod_read_half(buffer+0x16)<<16);
        gpu_write_gp0(psx_mod_read_half(buffer+0x18)|(uint32_t)psx_mod_read_half(buffer+0x1A)<<16);
        psx_mod_counter_add("mmx5.coop.intro-full-white-presents",1);
        local_view_call=0;
        cpu->gpr[2]=result;return 1;
    }
    int menu=personal_menu_present(),returning=personal_menu_world_return();
    int terminal=terminal_death_present();
    int local_menu=menu && !psx_mod_netplay_is_active();
    if(!local_split && !psx_mod_netplay_is_active())psx_mod_set_local_pair_presentation(local_menu);
    if(local_split && !local_aspect_set)
        local_aspect_set=apply_local_split_aspect();
    int paired_incoming=paired_stage_arrival();
    int shared_scene=shared_scene_pair_present() || duel_fill_pair_present();
    /* Native delayed health freezes the world, not either saved view.
     * Keep publishing the draw-only pair throughout the native heal. */
    if((!menu && !split_available()) || local_view_call || projected || busy ||
       (!menu && scene_owner && !scene_return_present() && !shared_scene) ||
       (!menu && !returning && !terminal && psx_mod_read_byte(PLAY+1)) || psx_mod_read_byte(PLAY+0xF) ||
       cpu->gpr[31]!=0x80011E50)return 0;
    if(cpu->gpr[4]!=buffer+0xAC)return 0;
    local_view_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    guest(cpu,0x80068634,0,0);
    int port=psx_netplay_local_port();unsigned seat=port>=0 && port<2?(unsigned)port:0;
    local_view_seat=menu?seat:split_view_seat(seat);
    local_hud_seat=seat;
    PSXModRenderPass rectangle={sizeof rectangle,0,psx_mod_read_half(buffer+0x14),psx_mod_read_half(buffer+0x16),
        psx_mod_read_half(buffer+0x18),psx_mod_read_half(buffer+0x1A)};
    LocalViewContext context;local_context_save(&context);
    uint32_t before=state_digest();
    int committed;
    if(local_split || local_menu) {
        PSXModStereoFrame pair={sizeof pair,1,rectangle.x,rectangle.y,rectangle.w,rectangle.h};
        enable_local_pair_presentation();
        committed=psx_mod_render_local_pair(cpu,&pair,draw_local_pair,&buffer);
        psx_mod_counter_add(committed?"mmx5.coop.local-pair-committed":"mmx5.coop.local-pair-refused",1);
        if(paired_incoming && committed)psx_mod_counter_add("mmx5.coop.spawn-pair-committed",1);
    }else committed=psx_mod_render_local_view(cpu,&rectangle,draw_local_view,&buffer);
    /* Restore on an aborted callback/watchdog path too. */
    local_context_restore(&context);
    if(state_digest()!=before)psx_mod_counter_add("mmx5.coop.local-view-state-leak",1);
    psx_mod_counter_add(committed?"mmx5.coop.local-view-committed":"mmx5.coop.local-view-refused",1);
    if(committed)psx_mod_counter_add(local_view_seat?"mmx5.coop.local-view-seat-1":"mmx5.coop.local-view-seat-0",1);
    local_view_call=0;cpu->gpr[2]=result;return 1;
}
static void activate(void) {
    mmx5_diagnostics_reset();
#ifndef PSX_NO_DEBUG_TOOLS
    fixture_location_pending=fixture_stage=fixture_section=fixture_checkpoint=0;fixture_scenery_pending=0;
#endif
    char option[16];
    unsigned ultimate=psx_mod_current_option_value("ultimate_armor_x",option,sizeof option) && !strcmp(option,"true");
    unsigned black=psx_mod_current_option_value("black_zero",option,sizeof option) && !strcmp(option,"true");
    mmx5_coop_forms_configure((int)ultimate,(int)black);
    selected_x_armor=1;selected_zero_form=mmx5_coop_forms_requested(1);
    armor_selection_seen=armor_ui_call=armor_selecting_seat=armor_list_seat=0;
    memset(armor_previous_input,0,sizeof armor_previous_input);
    bindings_seat=bindings_call=bindings_name_call=0;memset(bindings_previous,0,sizeof bindings_previous);
    armor_roster_active=armor_native_count=0;memset(armor_native_roster,0,sizeof armor_native_roster);
    x_asset_source=mmx5_coop_armor_source(mmx5_coop_forms_roster(0,selected_x_armor));
    keep_zero=!psx_mod_netplay_is_active() &&
        psx_mod_current_option_value("keep_zero",option,sizeof option) && !strcmp(option,"true");
    both_door_players=psx_mod_current_option_value("door_players",option,sizeof option) &&
        !strcmp(option,"both");
    separate_etanks=psx_mod_current_option_value("separate_etanks",option,sizeof option) &&
        !strcmp(option,"true");
    personal_menu_reset();
    etanks_initialized=0;memset(private_tank_contents,0,sizeof private_tank_contents);
        duel_story_character=0;
    boss_scaling=psx_mod_current_option_value("boss_scaling",option,sizeof option) && !strcmp(option,"true");
    boss_per_player=!(psx_mod_current_option_value("boss_cooldowns",option,sizeof option) && !strcmp(option,"shared"));
    balance_reset();
    hud_side_by_side=psx_mod_current_option_value("hud_layout",option,sizeof option) &&
        !strcmp(option,"side_by_side");
    if(!psx_mod_current_option_value("cameras",option,sizeof option))strcpy(option,"unified");
    split_views=(unsigned)mmx5_coop_views_requested(psx_mod_netplay_is_active(),option);
    local_split=!psx_mod_netplay_is_active() &&
        psx_mod_current_option_value("local_cameras",option,sizeof option) && !strcmp(option,"split");
    local_split_wide=local_split &&
        psx_mod_current_option_value("local_split_aspect",option,sizeof option) && !strcmp(option,"16:9");
    split_views|=local_split;local_aspect_set=0;
    if(local_split) {
        (void)apply_local_split_aspect();
        enable_local_pair_presentation();
    }else psx_mod_set_local_pair_presentation(0);
    split_valid=split_owner=split_camera_call=split_scan_call=split_supplemental=split_bounds_call=0;
    local_view_call=local_view_seat=local_hud_seat=0;split_table=0;
    ai_pool_call=ai_call=ai_owner=ai_suspended=0;ai_pool=ai_pending=ai_function=0;ai_first_freeze=0;
    ai_camera_restore=NULL;
    memset(&dinorex_sequence,0,sizeof dinorex_sequence);
    memset(&glow_stair,0,sizeof glow_stair);
    spatial_call=0;
    personal_effect_call=personal_visual_call=0;memset(personal_effect_owner,0,sizeof personal_effect_owner);
    memset(personal_visual_ticks,0,sizeof personal_visual_ticks);
    area_call=0;area_actor=area_function=0;
    memset(split_cameras,0,sizeof split_cameras);memset(split_visits,0,sizeof split_visits);
    for(unsigned i=0;i<view_bank_count;++i) {free(view_banks[i].pixels);view_banks[i].pixels=NULL;}
    view_bank_count=view_ui_bank_count=0;
    for(unsigned i=0;i<bank_count;++i) {free(banks[i].pixels);banks[i].pixels=NULL;}
    bank_count=ui_bank_count=0;
    busy=projected=enrolled=failed=ready=ticks=world_ticks=camera_call=0;previous_input=0;
    second_terrain_pending=0;
    if(!mmx5_coop_audio_activate())failed=1;
    stage_call=player_call=world_owner=render_call=0;mmx5_coop_lifecycle_init(&life);
    render_primary=0;duel_render_art=NULL;asset_generation=1;primary_packets=0;
    hud_call=hud_seat=hud_icon_call=0;hud_fade=128;
    pause_owner=-1;p2_start_previous=0;upload_call=menu_active=0;
    for(unsigned i=0;i<menu_backup_count;++i) {free(menu_backups[i].pixels);menu_backups[i].pixels=NULL;}
    menu_backup_count=0;
    camera_update_call=0;
    ever_enrolled=last_stage=last_section=section_pending=stage_departed=0;
    hit_count=allocation_call=solid_call=contact_call=0;
    duel_contact_call=duel_native_contact=duel_attacker=duel_transfer_failed=duel_native_injury=0;
    duel_opponent_call=duel_text_call=duel_story_character=0;
    memset(hit_history,0,sizeof hit_history);memset(solid_history,0,sizeof solid_history);
    ride_call=ride_spawn_call=ride_deferred_slots=0;
    memset(car_source_owner,0,sizeof car_source_owner);
    for(unsigned i=0;i<2;++i) {
        car_mirror[i]=psx_mod_alloc_guest_memory(0xB0,16);if(!car_mirror[i])failed=1;
    }
    pickup_call=0;memset(pickup_owners,0,sizeof pickup_owners);
    scene_owner=scene_phase=scene_call=door_call=paired_victory=scene_corridor_arrow=scene_capsule=0;scene_y=0;memset(scene_body,0,sizeof scene_body);
    memset(&story_duel,0,sizeof story_duel);
    memset(&both_door,0,sizeof both_door);door_wait=door_traversal_call=door_terrain_call=0;
    join_phase=0;join_y=0;memset(returning_body,0,sizeof returning_body);
    effect_allocate_call=effect_init_call=0;memset(death_origins,0,sizeof death_origins);
    memset(landing_effects,0,sizeof landing_effects);fixture_sequence=0;
#ifndef PSX_NO_DEBUG_TOOLS
    fixture_damage_actor=fixture_damage_stage=fixture_damage_kind=0;
#endif
#ifdef MMX5_COOP_TEST_CHEATS
    test_chord=test_god_mask=test_contact_mask=test_commands=0;
    psx_mod_counter_add("mmx5.coop.test-cheats-enabled",1);
#endif
    memset(&first,0,sizeof first);memset(&second,0,sizeof second);
    for(unsigned c=0;c<MMX5_COOP_ARMOR_SOURCES;++c) {
        free(source_assets[c].graphics);free(source_assets[c].messages);free(source_assets[c].ui_pixels);
        memset(&source_assets[c],0,sizeof source_assets[c]);
    }
    for(unsigned c=0;c<2;++c) {
        memset(&assets[c],0,sizeof assets[c]);
        uint32_t memory=psx_mod_alloc_guest_memory(0x58000,16);
        assets[c].sprites=memory;assets[c].assembly=memory+0x40000;
        assets[c].colors=memory+0x4C000;assets[c].palette=memory+0x50000;
        assets[c].menu_colors=memory+0x54000;assets[c].menu_layout=memory+0x54400;
        if(!memory)failed=1;
    }
    memset(&duel_x_art,0,sizeof duel_x_art);
    duel_x_art.assembly=psx_mod_alloc_guest_memory(0xC000,16);
    if(!duel_x_art.assembly)failed=1;
    for(unsigned c=0;c<2;++c) {
        assets[c].message_root=psx_mod_alloc_guest_memory(0x12000,16);
        if(!assets[c].message_root)failed=1;
    }
    arena=psx_mod_alloc_texture_packet_memory(FRAME_BYTES*2,16);
    primary_arena=psx_mod_alloc_texture_packet_memory(FRAME_BYTES*2,16);
    hud_arena=psx_mod_alloc_texture_packet_memory(0x10000,16);
    diagnostic=psx_mod_alloc_guest_memory(0x1000,16);
    armor_empty_parts=psx_mod_alloc_guest_memory(4,4);
    armor_name_frame=psx_mod_alloc_guest_memory(64,4);
    bindings_name_frame=psx_mod_alloc_guest_memory(256,4);
    if(armor_empty_parts)psx_mod_write_word(armor_empty_parts,0);
    if(!arena || !primary_arena || !hud_arena || !diagnostic || !armor_empty_parts || !armor_name_frame || !bindings_name_frame)failed=1;
    extra_ride_record=psx_mod_alloc_guest_memory(8,4);if(!extra_ride_record)failed=1;
    /* Host residency is available even when the first action is loading a
     * state. Guest art/VAB initialization still belongs to native enrollment. */
    if(!failed && !load_assets(0))failed=1;
    if(!failed) {
        uint32_t assembly=duel_x_art.assembly;
        duel_x_art=source_assets[2];duel_x_art.sprites=assets[0].sprites;
        duel_x_art.assembly=assembly;duel_x_art.colors=assets[0].colors;
        duel_x_art.palette=assets[0].palette;
        project(assembly,duel_x_art.graphics+0x40000,0xC000);
    }
    psx_mod_set_texture_bank_resolver(snapshot_resolve_bank);
    psx_mod_set_rewind_blocked(psx_mod_netplay_is_active());
    psx_mod_set_savestate_blocked(psx_mod_netplay_is_active());
    psx_mod_counter_add("mmx5.coop.independent-menus-effective",1);
    psx_mod_counter_add("mmx5.coop.private-etanks-effective",separate_etanks);
    psx_mod_counter_add("mmx5.coop.activated",1);
    psx_mod_counter_add("mmx5.coop.diagnostic-address",diagnostic);
    psx_mod_counter_add("mmx5.coop.personal-effects-diagnostic-version",1);
    psx_mod_counter_add("mmx5.coop.hud-arena-address",hud_arena);
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
#include "mmx5_coop_snapshot.inc"
static uint32_t state_digest(void) {
    uint32_t hash=2166136261u;
#ifdef MMX5_COOP_TEST_CHEATS
    hash=hash_word(hash,test_chord);hash=hash_word(hash,test_god_mask);
    hash=hash_word(hash,test_contact_mask);hash=hash_word(hash,test_commands);
#endif
    const unsigned words[]={ready,enrolled,failed,counterpart,projected,busy,world_owner,keep_zero,
        separate_etanks,etanks_initialized,personal_active,personal_initialized,personal_eligible,
        boss_scaling,boss_per_player,
        x_asset_source,selected_x_armor,selected_zero_form,armor_selection_seen,
        armor_selecting_seat,armor_list_seat,armor_previous_input[0],armor_previous_input[1],
        armor_native_count,armor_roster_active,bindings_seat,bindings_previous[0],bindings_previous[1],
        mmx5_coop_forms_enabled(0),mmx5_coop_forms_enabled(1),
        split_views,split_valid,split_owner,split_table,
        ai_pool_call,ai_call,ai_owner,ai_suspended,ai_pool,ai_pending,ai_function,ai_first_freeze,
        area_call,area_actor,area_function,personal_effect_call,
        ever_enrolled,last_stage,last_section,section_pending,stage_departed,previous_input,
        life.status[0],life.status[1],life.select_frames,life.select_armed,life.select_previous,life.wipe,
        join_phase,(uint32_t)join_y,(unsigned)(pause_owner+1),(unsigned)p2_start_previous,menu_active,
        scene_owner,scene_phase,paired_victory,scene_corridor_arrow,scene_capsule,(uint32_t)scene_y,both_door_players,door_wait,door_traversal_call,door_terrain_call,
        both_door.actor,(uint16_t)both_door.old_min,(uint16_t)both_door.old_max,
        (uint16_t)both_door.target_min,(uint16_t)both_door.target_max,(uint16_t)both_door.right,
        both_door.stage,both_door.section,both_door.phase,both_door.owner,both_door.owns_lock,
        story_duel.door,story_duel.opponent,story_duel.x,story_duel.y,story_duel.box,story_duel.damage_table,
        story_duel.phase,story_duel.entrant,story_duel.character[0],story_duel.character[1],
        story_duel.facing[0],story_duel.facing[1],story_duel.token[0],story_duel.token[1],
        story_duel.winner,duel_story_character,story_duel.intro_complete,story_duel.arrived,
        story_duel.health[0],story_duel.health[1]};
    for(unsigned i=0;i<sizeof words/sizeof words[0];++i)hash=hash_word(hash,words[i]);
    hash=hash_bytes(hash,armor_native_roster,sizeof armor_native_roster);
    hash=hash_word(hash,dinorex_sequence.actor);hash=hash_word(hash,dinorex_sequence.owner);
    hash=hash_bytes(hash,dinorex_sequence.camera,sizeof dinorex_sequence.camera);
    hash=hash_word(hash,glow_stair.actor);hash=hash_word(hash,glow_stair.valid);
    hash=hash_bytes(hash,glow_stair.pose,sizeof glow_stair.pose);
    for(unsigned seat=0;seat<2;++seat) {
        hash=hash_bytes(hash,personal_menus[seat].native,sizeof personal_menus[seat].native);
        hash=hash_word(hash,personal_menus[seat].original_weapon);
        hash=hash_word(hash,personal_menu_previous[seat]);
    }
    for(unsigned i=0;i<48;++i) {
        const BossBalanceState *s=&boss_balance[i];const Mmx5CoopDamageBank *d=&s->damage;
        hash=hash_word(hash,boss_generations[i]);hash=hash_word(hash,d->actor);
        hash=hash_word(hash,d->generation);hash=hash_word(hash,d->bank);
        hash=hash_word(hash,d->serial);hash=hash_word(hash,d->remainder);
        hash=hash_word(hash,s->controller);hash=hash_word(hash,s->vulnerable_table);
        hash=hash_word(hash,s->descriptor);hash=hash_word(hash,s->window);
        hash=hash_word(hash,s->window_phase);
        hash=hash_word(hash,s->cooldown.ticks[0]);hash=hash_word(hash,s->cooldown.ticks[1]);
    }
    hash=hash_bytes(hash,(const uint8_t *)private_tank_contents,sizeof private_tank_contents);
    hash=hash_bytes(hash,second.body,sizeof second.body);
    hash=hash_bytes(hash,second.shots,sizeof second.shots);
    hash=hash_bytes(hash,second.glow,sizeof second.glow);
    hash=hash_bytes(hash,second.trails,sizeof second.trails);
    hash=hash_bytes(hash,first.car,sizeof first.car);hash=hash_bytes(hash,second.car,sizeof second.car);
    hash=hash_bytes(hash,car_source_owner,sizeof car_source_owner);
    hash=hash_bytes(hash,returning_body,sizeof returning_body);
    hash=hash_bytes(hash,scene_body,sizeof scene_body);
    hash=hash_bytes(hash,story_duel.native_body,sizeof story_duel.native_body);
    hash=hash_bytes(hash,story_duel.colors,sizeof story_duel.colors);
    hash=hash_bytes(hash,personal_effect_owner,sizeof personal_effect_owner);
    hash=hash_bytes(hash,split_cameras[0],sizeof split_cameras);hash=hash_bytes(hash,split_visits,sizeof split_visits);
    if(projected) {
        hash=hash_bytes(hash,first.body,sizeof first.body);
        hash=hash_bytes(hash,first.shots,sizeof first.shots);
        hash=hash_bytes(hash,first.glow,sizeof first.glow);
        hash=hash_bytes(hash,first.trails,sizeof first.trails);
        hash=hash_bytes(hash,saved_play,sizeof saved_play);hash=hash_bytes(hash,saved_camera,sizeof saved_camera);
        for(unsigned i=0;i<6;++i)hash=hash_word(hash,resources[i]);
    }
    hash=hash_word(hash,hit_count);
    for(unsigned i=0;i<hit_count;++i) {
        hash=hash_word(hash,hit_history[i].actor);hash=hash_bytes(hash,hit_history[i].token,2);
        hash=hash_word(hash,hit_history[i].capture_owner);
    }
    for(unsigned i=0;i<64;++i) {
        hash=hash_word(hash,solid_history[i].sides);hash=hash_word(hash,solid_history[i].carried);
        hash=hash_word(hash,solid_history[i].car_sides);hash=hash_word(hash,solid_history[i].car_carried);
        hash=hash_word(hash,solid_history[i].controller_p2);
    }
    hash=hash_word(hash,ride_call);
    hash=hash_word(hash,ride_spawn_call);hash=hash_word(hash,ride_deferred_slots);
    hash=hash_word(hash,extra_ride_record);
    if(extra_ride_record) {
        uint8_t record[8];capture(extra_ride_record,record,sizeof record);
        hash=hash_bytes(hash,record,sizeof record);
    }
    for(unsigned i=0;i<128;++i) {
        DeathOrigin *o=&death_origins[i];hash=hash_word(hash,o->valid);
        if(o->valid) {hash=hash_word(hash,o->x);hash=hash_word(hash,o->y);
            hash=hash_word(hash,o->assembly);hash=hash_word(hash,o->layer);}
        LandingEffect *landing=&landing_effects[i];hash=hash_word(hash,landing->owner);
        hash=hash_word(hash,landing->sprites);hash=hash_word(hash,landing->assembly);
        hash=hash_word(hash,pickup_owners[i].actor);
        if(pickup_owners[i].actor)hash=hash_word(hash,pickup_owners[i].seat);
    }
    hash=hash_word(hash,menu_backup_count);
    for(unsigned i=0;i<menu_backup_count;++i) {
        const MenuBackup *r=&menu_backups[i];hash=hash_word(hash,r->x);hash=hash_word(hash,r->y);
        hash=hash_word(hash,r->w);hash=hash_word(hash,r->h);
        for(unsigned j=0;j<r->w*r->h;++j)hash=hash_word(hash,r->pixels[j]);
    }
    return mmx5_coop_audio_digest(hash);
}
#ifndef MMX5_COOP_NETPLAY_COMPATIBILITY
#define MMX5_COOP_NETPLAY_COMPATIBILITY "mmx5-coop-delay-unconfigured"
#endif
typedef struct {
    uint32_t site,load,delay,instruction;
    uint8_t result,base;
} LifeCapacityRead;
/* Only consumers of the earned capacity are scoped. Card serialization and
 * the native Heart/Life Up read-modify-write continue using real progress. */
static const LifeCapacityRead life_capacity_reads[]={
    {0x80025D44u,0x80620047u,0,0x2442FFE0u,2,3},
    {0x80026030u,0x80620047u,0,0x2442FFE0u,2,3},
    {0x8002632Cu,0x80540047u,0x080098D4u,0x3C028007u,20,2},
    {0x80033BA4u,0x80650047u,0x3042007Fu,0x0045102Au,5,3},
    {0x80033BF4u,0x80650047u,0x3042007Fu,0x0045102Au,5,3},
    {0x800341C0u,0x80640047u,0x00023603u,0x0086182Au,4,3},
    {0x800341FCu,0x80650047u,0x9103005Cu,0x1045000Cu,5,0},
    {0x800342C0u,0x80430047u,0,0x0066102Au,3,2},
    {0x80039C5Cu,0x90430047u,0x03E00008u,0xA083005Cu,3,2},
    {0x8003BC30u,0x90830047u,0x80450053u,0x262400A8u,3,4},
    {0x8003CFD4u,0x90430047u,0x34A55556u,0x00031E00u,3,2},
    {0x8003D0C8u,0x90430047u,0x34A55556u,0x00031E00u,3,2},
    {0x80040128u,0x90430047u,0x34A55556u,0x00031E00u,3,2},
    {0x80041E64u,0x90430047u,0x34A55556u,0x00031E00u,3,2},
    {0x80053AC8u,0x80830000u,0x30A2007Fu,0x0043102Au,3,4},
    {0x80053AF0u,0x90620000u,0x08014EBEu,0x00E21025u,2,3},
    {0x80053E94u,0x80450047u,0x3063007Fu,0x0065182Au,5,2},
    {0x80053EF8u,0x80440047u,0x90420047u,0x0064182Au,4,0},
    {0x800541F8u,0x90450047u,0x0C014F8Fu,0x24060004u,5,2}
};
static void life_capacity_read(CPUState *cpu,uint32_t address) {
    if(failed || psx_mod_read_byte(PLAY)!=10)return;
    address=0x80000000u|(address&0x1FFFFFFFu);
    unsigned x=psx_mod_read_byte(PLAY+0x47),z=psx_mod_read_byte(PLAY+0x48);
    if(!x || !z || x>127 || z>127)return;
    unsigned maximum=duel_scope()?DUEL_HP:x>z?x:z;
    for(unsigned i=0;i<sizeof life_capacity_reads/sizeof life_capacity_reads[0];++i) {
        const LifeCapacityRead *r=&life_capacity_reads[i];
        if(r->site!=address)continue;
        if(psx_mod_read_word(address-8)!=r->load ||
           psx_mod_read_word(address-4)!=r->delay ||
           psx_mod_read_word(address)!=r->instruction)return;
        if(r->base) {
            uint32_t source=cpu->gpr[r->base]+(uint32_t)(int32_t)(int16_t)r->load;
            if((source&0x1FFFFFFFu)!=((PLAY+0x47u)&0x1FFFFFFFu) &&
               (source&0x1FFFFFFFu)!=((PLAY+0x48u)&0x1FFFFFFFu))return;
        }
        cpu->gpr[r->result]=maximum;
        /* This native pickup reads the same capacity into two registers;
         * the second load has already replaced its base register. */
        if(address==0x80053EF8u)cpu->gpr[2]=maximum;
        return;
    }
}
PSX_MOD_CONSTRUCTOR(mmx5_register_coop) {
    psx_mod_register_function_filter_plugin(ID,0x8003D688,bindings_mapper);
    psx_mod_register_function_filter_plugin(ID,0x8002B8F0,bindings_editor);
    psx_mod_register_instruction_plugin(ID,0x8004D2D0u,0x14A20005u,bindings_icon);
    psx_mod_register_instruction_plugin(ID,0x8003E20Cu,0x30420040u,duel_giga_input);
    psx_mod_register_instruction_plugin(ID,0x800F4150u,0xA462A226u,dinorex_native_camera_focus);
    for(unsigned i=0;i<sizeof life_capacity_reads/sizeof life_capacity_reads[0];++i)
        psx_mod_register_instruction_plugin(ID,life_capacity_reads[i].site,
            life_capacity_reads[i].instruction,life_capacity_read);
    static const PSXModSnapshotProvider snapshot={18,snapshot_bytes,snapshot_write,
        snapshot_validate,snapshot_read,snapshot_ready};
    psx_mod_register_snapshot_provider(ID,&snapshot);
    psx_mod_register_savestate_plugin(ID,snapshot_restored);
    static const PSXModNetplayProfile profile={
        .plugin_id=ID,.compatibility_id=MMX5_COOP_NETPLAY_COMPATIBILITY,
        .skip_bios_intro=1,.fixed_aspect_mask=1u,.widescreen_plugin_id="mmx5.widescreen",
        .state_digest=state_digest,.requires_retained_texture_banks=1,
        .settings_package_id=ID,.settings_feature_id="coop",
        .renderer_option_id="cameras",.renderer_option_value="split",
        .host_profiles=mmx5_coop_host_profiles};
    psx_mod_register_netplay_profile(&profile);
    psx_mod_register_activation_plugin(ID,activate);
    psx_mod_register_vblank_plugin(ID,observe_stage);
    psx_mod_register_function_entry_plugin(ID,0x8003BAC8,reset);
    psx_mod_register_function_filter_plugin(ID,0x800F0C80,armor_stage_ui);
    psx_mod_register_instruction_plugin(ID,0x800F0028,0x30820840,armor_virtual_entry);
    psx_mod_register_instruction_plugin(ID,0x800F02E4,0x94830004,armor_virtual_entry);
    psx_mod_register_instruction_plugin(ID,0x800F1144,0x3C03800D,armor_virtual_entry);
    static const uint32_t parts_sites[][2]={
        {0x8003D608u,0x8C430088u},{0x8003D7BCu,0x8C430088u},
        {0x800F0538u,0x8C630000u},{0x800F07DCu,0x8CA40000u},
        {0x800F0810u,0xACA20000u},{0x800F083Cu,0x8C430000u},
        {0x800F3D90u,0x8C820088u}
    };
    for(unsigned i=0;i<sizeof parts_sites/sizeof parts_sites[0];++i)
        psx_mod_register_memory_address_plugin(ID,parts_sites[i][0],parts_sites[i][1],armor_parts_address);
    psx_mod_register_function_entry_plugin(ID,0x8003D5E0,form_body_init);
    psx_mod_register_function_entry_plugin(ID,0x8003D784,form_body_init);
#ifndef PSX_NO_DEBUG_TOOLS
    psx_mod_register_function_entry_plugin(ID,0x80020B80,fixture_mission_begin);
    psx_mod_register_function_entry_plugin(ID,0x8002ACC4,fixture_checkpoint_begin);
#endif
    psx_mod_register_function_filter_plugin(ID,0x80022084,attacks_tick);
    psx_mod_register_function_filter_plugin(ID,0x80022D98,personal_menu_world);
    psx_mod_register_function_filter_plugin(ID,0x80024C60,render_hook);
    psx_mod_register_function_entry_plugin(ID,0x80024B88,private_partner_before_sort);
    psx_mod_register_function_filter_plugin(ID,0x80025240,cull_sprite_draw);
    psx_mod_register_function_filter_plugin(ID,0x8002553C,cull_sprite_draw);
    /* Native24698 draws Fourth Armor separately after PLAYER, through
     * 27370 ->26CAC. It must obey the same view cull as the base body. */
    psx_mod_register_function_filter_plugin(ID,0x80027370,cull_sprite_draw);
    psx_mod_register_function_filter_plugin(ID,0x80031DD0,contact);
    psx_mod_register_function_filter_plugin(ID,0x80031F60,duel_private_contact);
    psx_mod_register_function_filter_plugin(ID,0x80031670,duel_injury_amount);
    psx_mod_register_function_filter_plugin(ID,0x800321EC,duel_projectile_result);
    psx_mod_register_function_filter_plugin(ID,0x800F4D7C,duel_opponent_tick);
    psx_mod_register_function_filter_plugin(ID,0x800F8B60,duel_opponent_tick);
    psx_mod_register_function_filter_plugin(ID,0x800317B8,contact);
    psx_mod_register_function_filter_plugin(ID,0x8002A720,camera_target);
    psx_mod_register_function_filter_plugin(ID,0x8002A7E0,camera_target);
    psx_mod_register_function_filter_plugin(ID,0x8002A440,camera_update);
    psx_mod_register_function_filter_plugin(ID,0x8002FB58,player_terrain);
    psx_mod_register_function_filter_plugin(ID,0x800F7618,glow_teleport_admission);
    psx_mod_register_function_filter_plugin(ID,0x8002A554,split_camera_finish);
    psx_mod_register_instruction_plugin(ID,0x8002A5FC,0xA4430034,shared_quake_command);
    psx_mod_register_instruction_plugin(ID,0x8002A638,0xA4430034,shared_quake_command);
    psx_mod_register_instruction_plugin(ID,0x800F4620,0x1060000D,broken_hover_rider);
    psx_mod_register_instruction_plugin(ID,0x800F7744,0x1060000D,broken_hover_rider);
    psx_mod_register_instruction_plugin(ID,0x800FC6A0,0x1060000D,broken_hover_rider);
    psx_mod_register_instruction_plugin(ID,0x800FD63C,0xA0510044,whale_final_follow);
    psx_mod_register_instruction_plugin(ID,0x800EEC0C,0x90A30005,shared_intro_layer_command);
    psx_mod_register_instruction_plugin(ID,0x800FCE74,0xA06200A0,shared_final_atlas_command);
    psx_mod_register_function_filter_plugin(ID,0x8002AE50,split_scan);
    psx_mod_register_function_filter_plugin(ID,0x8002AFB4,narrow_lifetime_admission);
    psx_mod_register_function_filter_plugin(ID,0x8002B4E4,split_allocate);
    psx_mod_register_instruction_plugin(ID,0x8002B39C,0xA2420000,split_spawned);
    psx_mod_register_function_filter_plugin(ID,0x8002D800,split_actor_bounds);
    psx_mod_register_function_filter_plugin(ID,0x8002D938,split_actor_bounds);
    psx_mod_register_function_filter_plugin(ID,0x800F7B14,grizzly_chain_tick);
    psx_mod_register_function_filter_plugin(ID,0x8002D89C,split_actor_bounds);
    psx_mod_register_function_filter_plugin(ID,0x8002D9EC,split_actor_bounds);
    psx_mod_register_function_filter_plugin(ID,0x8002DAA0,split_actor_bounds);
    psx_mod_register_instruction_plugin(ID,0x800F1200,0x14400011,intro_scenery_checkpoint);
    psx_mod_register_function_filter_plugin(ID,0x800F3780,dinorex_wall_camera);
    psx_mod_register_function_filter_plugin(ID,0x800F36F8,dinorex_fire_survivor);
    psx_mod_register_function_filter_plugin(ID,0x800F3E7C,dinorex_fire_wall_burst);
    psx_mod_register_function_filter_plugin(ID,0x800F66D4,dinorex_upper_wall_camera);
    psx_mod_register_function_filter_plugin(ID,0x800F43E8,dinorex_dragon_survivor);
    psx_mod_register_function_filter_plugin(ID,0x800F6B60,dinorex_lava_camera);
    psx_mod_register_function_filter_plugin(ID,0x800F2AE8,dinorex_fire_contact);
    psx_mod_register_instruction_plugin(ID,0x800F26D4,0x28420EB1,section_exit_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F39E8,0x28421134,section_exit_predicate);
    psx_mod_register_function_filter_plugin(ID,0x80068C28,present_local_view);
    psx_mod_register_function_filter_plugin(ID,0x80021F64,ai_pool_tick);
    psx_mod_register_function_filter_plugin(ID,0x800221B0,ai_pool_tick);
    psx_mod_register_function_filter_plugin(ID,0x800F2448,rose_capture_tick);
    psx_mod_register_function_filter_plugin(ID,0x800F6EFC,rose_capture_tick);
    psx_mod_register_function_filter_plugin(ID,0x800FE0C8,rose_capture_tick);
    psx_mod_register_function_filter_plugin(ID,0x8005609C,spatial_stage_actor);
    psx_mod_register_function_filter_plugin(ID,0x80056AA0,spatial_stage_actor);
    psx_mod_register_function_filter_plugin(ID,0x80056E88,spatial_stage_actor);
    psx_mod_register_instruction_plugin(ID,0x80022044,0x00A0F809,ai_redirect);
    psx_mod_register_instruction_plugin(ID,0x80022290,0x00A0F809,ai_redirect);
    psx_mod_register_instruction_plugin(ID,0x800F4D90,0x14600005,story_roster_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F8BDC,0x14600031,story_roster_predicate);
    psx_mod_register_instruction_plugin(ID,0x80056848,0x14600004,story_roster_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F2358,0x00831023,falling_hazard_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F2474,0x00A31023,falling_hazard_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F99D0,0x00831023,route_solid_contact_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F9BB8,0x00641023,route_solid_contact_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F9D34,0x00831023,route_solid_contact_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F2D60,0x00641023,route_solid_contact_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F8898,0x00831023,route_solid_contact_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F8A80,0x00641023,route_solid_contact_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F8BFC,0x00831023,route_solid_contact_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F22D4,0x14830002,kraken_switch_retirement);
    psx_mod_register_instruction_plugin(ID,0x800EEE0C,0x10400015,rose_floor_predicate);
    psx_mod_register_instruction_plugin(ID,0x800EEDD4,0x14400016,refight_hub_seal);
    psx_mod_register_instruction_plugin(ID,0x800EEE1C,0x286203E0,rose_floor_predicate);
    psx_mod_register_instruction_plugin(ID,0x800EEE34,0x240303C0,rose_floor_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F59A0,0x00A31023,disappearing_platform_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F4CE4,0x14830007,up_support_resume_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F4D1C,0x14830007,up_support_resume_predicate);
    psx_mod_register_instruction_plugin(ID,0x800F3CA8,0x2463FFF0,camera_emitter_origin);
    psx_mod_register_instruction_plugin(ID,0x800F3D60,0x2442FFF0,camera_emitter_origin);
    psx_mod_register_instruction_plugin(ID,0x800F2874,0x0C00B7DE,camera_emitter_origin);
    psx_mod_register_instruction_plugin(ID,0x800F28A4,0x2442FF88,camera_emitter_origin);
    psx_mod_register_instruction_plugin(ID,0x800F28CC,0x24420078,camera_emitter_origin);
    psx_mod_register_instruction_plugin(ID,0x800F2978,0x28420A60,camera_emitter_origin);
    psx_mod_register_instruction_plugin(ID,0x800EECA8,0x0C00B7DE,water_head_round);
    psx_mod_register_instruction_plugin(ID,0x800F7F4C,0x2463FFEE,water_head_origin);
    psx_mod_register_instruction_plugin(ID,0x800F7F5C,0x14400004,water_head_origin);
    psx_mod_register_instruction_plugin(ID,0x800F7F68,0x0803DFDF,water_head_origin);
    psx_mod_register_instruction_plugin(ID,0x800F7F78,0x2442000C,water_head_origin);
    psx_mod_register_instruction_plugin(ID,0x800225D8,0x00A0F809,refight_pad_redirect);
    psx_mod_register_instruction_plugin(ID,0x800F1C28,0x00831023,chain_platform_proximity);
    psx_mod_register_instruction_plugin(ID,0x800F72D0,0x14400005,glow_stair_retirement);
    psx_mod_register_instruction_plugin(ID,0x800228D8,0x0060F809,area_redirect);
    psx_mod_register_instruction_plugin(ID,0x80022920,0x0060F809,area_redirect);
    psx_mod_register_instruction_plugin(ID,0x80032184,0x34638000,duel_damage);
    psx_mod_register_instruction_plugin(ID,0x80038C08,0xA0621C1C,fatal_pause_lock);
    psx_mod_register_instruction_plugin(ID,0x8003D088,0xA0431C1C,fatal_pause_lock);
    psx_mod_register_instruction_plugin(ID,0x800318F4,0xA0A2001C,fatal_pause_lock);
    psx_mod_register_instruction_plugin(ID,0x80031B50,0xA043001C,fatal_pause_lock);
    psx_mod_register_instruction_plugin(ID,0x80020760,0x10400003,duel_story_branch);
    psx_mod_register_instruction_plugin(ID,0x800F8870,0xA2020080,balance_recovery);
    psx_mod_register_instruction_plugin(ID,0x800F351C,0xA2220090,balance_recovery);
    psx_mod_register_instruction_plugin(ID,0x800F4E08,0xA202008A,balance_recovery);
    psx_mod_register_instruction_plugin(ID,0x800F7EC4,0xA462008E,balance_recovery);
    psx_mod_register_instruction_plugin(ID,0x800F9A00,0xA0620088,balance_recovery);
    psx_mod_register_instruction_plugin(ID,0x800F2828,0xA2020094,balance_recovery);
    psx_mod_register_instruction_plugin(ID,0x800F33A4,0xA2020084,balance_recovery);
    psx_mod_register_instruction_plugin(ID,0x800F53F8,0xA062008A,balance_recovery);
    psx_mod_register_instruction_plugin(ID,0x800EEB68,0xA2020084,balance_recovery);
    psx_mod_register_instruction_plugin(ID,0x800F7EF4,0xA443008E,balance_phase_timer);
    psx_mod_register_instruction_plugin(ID,0x800EEAA4,0xA2020084,balance_phase_timer);
    psx_mod_register_instruction_plugin(ID,0x800EEAD0,0xA2020084,balance_phase_timer);
    psx_mod_register_instruction_plugin(ID,0x800EEA7C,0xA2020084,balance_phase_timer);
    psx_mod_register_instruction_plugin(ID,0x800F34CC,0x2A020003,balance_reaction);
    psx_mod_register_instruction_plugin(ID,0x800F80D0,0x1062000C,balance_reaction);
    psx_mod_register_guest_function_plugin(ID,BALANCE_DISPATCH,balance_update);
    psx_mod_register_guest_function_plugin(ID,AI_DISPATCH,ai_update);
    psx_mod_register_guest_function_plugin(ID,AREA_DISPATCH,area_update);
    psx_mod_register_function_filter_plugin(ID,0x80035040,player_tick);
    psx_mod_register_function_filter_plugin(ID,0x80022960,ride_tick);
    psx_mod_register_function_filter_plugin(ID,0x800EE988,ride_factory);
    psx_mod_register_function_filter_plugin(ID,0x80020D5C,stage_tick);
    psx_mod_register_function_filter_plugin(ID,0x80034A04,personal_menu_tick);
    psx_mod_register_function_filter_plugin(ID,0x80018400,personal_menu_native_draw);
    psx_mod_register_function_filter_plugin(ID,0x80034E90,personal_menu_dead_commit);
    psx_mod_register_function_filter_plugin(ID,0x80013288,personal_menu_shared);
    psx_mod_register_function_filter_plugin(ID,0x800132F8,personal_menu_shared);
    psx_mod_register_function_filter_plugin(ID,0x800183B4,personal_menu_shared);
    psx_mod_register_function_filter_plugin(ID,0x800272B4,personal_menu_shared);
    psx_mod_register_function_filter_plugin(ID,0x8002D27C,effect_allocate);
    psx_mod_register_function_filter_plugin(ID,0x8004DB18,effect_init);
    psx_mod_register_function_filter_plugin(ID,0x8004C620,landing_effect_tick);
    psx_mod_register_function_filter_plugin(ID,0x80025828,draw_hud);
    psx_mod_register_function_filter_plugin(ID,0x80026080,hud_icon);
    psx_mod_register_function_filter_plugin(ID,0x800688E0,retained_upload);
    psx_mod_register_function_filter_plugin(ID,0x8002CA20,allocate_actor);
    psx_mod_register_function_filter_plugin(ID,0x8002C938,allocate_actor);
    psx_mod_register_function_filter_plugin(ID,0x8002D210,allocate_actor);
    psx_mod_register_function_filter_plugin(ID,0x8002D0E4,allocate_actor);
    psx_mod_register_function_filter_plugin(ID,0x8002D144,allocate_actor);
    psx_mod_register_function_filter_plugin(ID,0x8004F0B4,personal_deferred_effect);
    psx_mod_register_function_filter_plugin(ID,0x8004F38C,personal_deferred_effect);
    psx_mod_register_function_filter_plugin(ID,0x8004F3F4,personal_deferred_effect);
    psx_mod_register_function_filter_plugin(ID,0x8004F8E8,personal_deferred_effect);
    psx_mod_register_function_filter_plugin(ID,0x800EF0A0,personal_deferred_effect);
    psx_mod_register_function_filter_plugin(ID,0x80046900,personal_deferred_effect);
    psx_mod_register_function_filter_plugin(ID,0x8004C4DC,personal_deferred_effect);
    psx_mod_register_function_filter_plugin(ID,0x80050D04,personal_deferred_effect);
    psx_mod_register_function_filter_plugin(ID,0x80050F58,personal_deferred_effect);
    psx_mod_register_function_filter_plugin(ID,0x80022EDC,personal_visual_tick);
    psx_mod_register_function_filter_plugin(ID,0x80022F3C,personal_visual_tick);
    psx_mod_register_function_filter_plugin(ID,0x8003240C,solid_contact);
    /* Destructible terrain (including the bike blockades) queries shots
     * through32F70 instead of the ordinary enemy31DD0 path. */
    psx_mod_register_function_filter_plugin(ID,0x80032F70,contact);
    psx_mod_register_function_filter_plugin(ID,0x80054FE4,capsule_secret_init);
    psx_mod_register_function_filter_plugin(ID,0x80055D24,capsule_black_form);
    psx_mod_register_function_filter_plugin(ID,0x80055DEC,capsule_secret_return);
    psx_mod_register_function_filter_plugin(ID,0x80055798,capsule_gate);
    psx_mod_register_function_filter_plugin(ID,0x80055AD0,capsule_gate);
    psx_mod_register_function_filter_plugin(ID,0x8002385C,duel_text);
    psx_mod_register_function_filter_plugin(ID,0x80054000,pickup_collect);
    psx_mod_register_instruction_plugin(ID,0x80054048,0x90A2005C,pickup_armor_collider);
    psx_mod_register_function_filter_plugin(ID,0x800EF7A4,refight_pad_admission);
    psx_mod_register_function_filter_plugin(ID,0x800F588C,falling_slab_trigger);
    psx_mod_register_function_filter_plugin(ID,0x800EEAF0,dinorex_floor_seal);
    psx_mod_register_function_filter_plugin(ID,0x80028B18,coop_floor_map_copy);
    psx_mod_register_instruction_plugin(ID,0x800EEEDC,0x14400008,sigma1_final_floor_wait);
    psx_mod_register_function_filter_plugin(ID,0x800EF308,sigma1_floor_admission);
    psx_mod_register_function_filter_plugin(ID,0x800EF3AC,sigma1_floor_retire);
    psx_mod_register_function_filter_plugin(ID,0x8002C2C0,dark_hold_palette);
    psx_mod_register_function_filter_plugin(ID,0x8002BD8C,dark_hold_palette);
    psx_mod_register_function_filter_plugin(ID,0x800F7124,sigma1_laser_proximity);
    psx_mod_register_function_filter_plugin(ID,0x800F65C8,intro_structure_trigger);
    psx_mod_register_function_filter_plugin(ID,0x800F8170,intro_reunion_skip);
    psx_mod_register_function_filter_plugin(ID,0x800F26AC,corridor_arrow_admission);
    psx_mod_register_function_filter_plugin(ID,0x800EFA30,necrobat_platform_riders);
    psx_mod_register_instruction_plugin(ID,0x800EFA54,0x0064182A,necrobat_platform_reset_predicate);
    psx_mod_register_function_filter_plugin(ID,0x800F48B4,descending_support_trigger);
    psx_mod_register_function_filter_plugin(ID,0x800EFC34,midboss_admission);
    psx_mod_register_function_filter_plugin(ID,0x800EFF84,midboss_admission);
    psx_mod_register_function_filter_plugin(ID,0x800EFE20,midboss_admission);
    psx_mod_register_function_filter_plugin(ID,0x8003B65C,whale_drain_physics);
    psx_mod_register_function_filter_plugin(ID,0x800F7194,glow_stair_native);
    psx_mod_register_function_filter_plugin(ID,0x800EF584,whale_player_bounds);
    psx_mod_register_function_filter_plugin(ID,0x800F0260,whale_player_bounds);
    psx_mod_register_function_filter_plugin(ID,0x800F02CC,whale_player_bounds);
    psx_mod_register_function_filter_plugin(ID,0x800F48F8,later_support_controller);
    psx_mod_register_function_filter_plugin(ID,0x800F4968,later_support_controller);
    psx_mod_register_instruction_plugin(ID,0x800F4990,0x28420E21,ascending_support_predicate);
    psx_mod_register_function_filter_plugin(ID,0x800F3930,later_support_controller);
    psx_mod_register_function_filter_plugin(ID,0x800F4A5C,later_support_controller);
    psx_mod_register_function_filter_plugin(ID,0x80053A4C,pickup_tick);
    psx_mod_register_function_entry_plugin(ID,0x8003D29C,script_begin);
    psx_mod_register_function_filter_plugin(ID,0x8003B1A8,partner_victory_pose);
    psx_mod_register_function_filter_plugin(ID,0x80054BB8,door_tick);
    psx_mod_register_function_filter_plugin(ID,0x800549E8,both_door_query);
    psx_mod_register_function_filter_plugin(ID,0x80054D9C,both_door_traversal);
    psx_mod_register_function_filter_plugin(ID,0x80030BB4,both_door_map);
    psx_mod_register_function_filter_plugin(ID,0x80030AE4,both_door_map);
}


/* Read-only canonical contexts for the input recorder. A projected P2 body
 * occupies native PLAYER temporarily; consumers must retain the P1 label. */
int mmx5_coop_ready(void) {return ready && enrolled;}
int mmx5_coop_menu_secondary(void) {
    return personal_menu_call && personal_menu_seat && !personal_menu_secondary_modal();
}
int mmx5_coop_projected(void) {return projected != 0;}
int mmx5_coop_split_views(void) {return split_views != 0;}
const uint8_t *mmx5_coop_first_body(void) {return first.body;}
const uint8_t *mmx5_coop_second_body(void) {return second.body;}
uint16_t mmx5_coop_input(unsigned seat) {return input(seat);}
uint32_t mmx5_coop_call(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1) {
    return guest(cpu,address,a0,a1);
}
int mmx5_coop_finish(CPUState *cpu,uint32_t result) {cpu->gpr[2]=result;return 1;}
