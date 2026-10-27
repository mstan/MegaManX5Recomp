/* Exercise the actual title walker, including variable retained caches, rather
 * than a duplicate test encoder. Unused gameplay entry points are discarded. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mod_plugins.h"
#undef PSX_MOD_CONSTRUCTOR
#define PSX_MOD_CONSTRUCTOR(name) static void name(void)
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned allocation_forbidden,allocations;
static void *checked_malloc(size_t n) {CHECK(!allocation_forbidden);++allocations;return malloc(n);}
static void *checked_calloc(size_t n,size_t m) {CHECK(!allocation_forbidden);++allocations;return calloc(n,m);}
static void *checked_realloc(void *p,size_t n) {CHECK(!allocation_forbidden);++allocations;return realloc(p,n);}
#define malloc checked_malloc
#define calloc checked_calloc
#define realloc checked_realloc
#include "../src/mods/mmx5_coop_plugin.c"

static Mmx5WidescreenViewContext wide;
static unsigned enabled_forms[2];
uint8_t psx_mod_read_byte(uint32_t address) {(void)address;CHECK(0);return 0;}
uint32_t psx_mod_read_word(uint32_t address) {(void)address;CHECK(0);return 0;}
unsigned mmx5_coop_forms_enabled(unsigned character) {
    CHECK(character<2);return enabled_forms[character];
}
static uint16_t resolved_id,resolved_pixel;
int psx_mod_define_texture_bank(uint16_t id,uint32_t width,uint32_t height,const uint16_t *pixels) {
    CHECK(width==16 && height==16 && pixels);resolved_id=id;resolved_pixel=pixels[0];return 1;
}
void mmx5_widescreen_view_save(Mmx5WidescreenViewContext *v) {*v=wide;}
void mmx5_widescreen_view_restore(const Mmx5WidescreenViewContext *v) {wide=*v;}
int mmx5_coop_audio_snapshot_ready(void) {return 1;}
void mmx5_coop_audio_snapshot(Mmx5SnapshotIO *s) {mmx5_snapshot_fixed(s,123);}
int psx_mod_local_view_scope(void) {return 0;}
void psx_dispatch_call(CPUState *cpu,uint32_t target,uint32_t ret) {(void)cpu;(void)target;(void)ret;CHECK(0);}
uint16_t sio_get_pad_buttons_slot(int slot) {(void)slot;CHECK(0);return 0;}
int mmx5_diagnostics_resolve_input(unsigned seat,uint16_t *buttons) {(void)seat;(void)buttons;CHECK(0);return 0;}

static void check_source_binding(unsigned character,unsigned source) {
    CharacterAssets *a=&assets[character],*native=&source_assets[source];
    CHECK(a->graphics==native->graphics && a->graphics_bytes==native->graphics_bytes);
    CHECK(a->messages==native->messages && a->message_bytes==native->message_bytes);
    CHECK(a->ui_pixels==native->ui_pixels && a->loaded==native->loaded);
    CHECK(a->record==native->record && a->identity==native->identity);
    CHECK(!memcmp(a->hud_colors,native->hud_colors,sizeof a->hud_colors));
}
static void form_source_roundtrip(const uint8_t *automatic,uint32_t len) {
    uint8_t *base=malloc(len),*ultimate=malloc(len),*bad=malloc(len);CHECK(base && ultimate && bad);
    /* With automatic X disabled, a genuine native Ultimate body may still
     * bind DAT93. Only the binding byte changes; guest ranges stay fixed. */
    enabled_forms[0]=0;x_asset_source=0;bind_character_source(0,0);
    CHECK(snapshot_write(base,len));
    x_asset_source=2;bind_character_source(0,2);CHECK(snapshot_write(ultimate,len));
    uint32_t selector=UINT32_MAX,differences=0;
    for(uint32_t i=0;i<len;++i)if(base[i]!=ultimate[i]) {selector=i;++differences;}
    CHECK(differences==1 && base[selector]==0 && ultimate[selector]==2);
    CHECK(snapshot_validate(base,len));
    allocation_forbidden=1;snapshot_read(base,len);allocation_forbidden=0;
    CHECK(x_asset_source==0);check_source_binding(0,0);check_source_binding(1,1);
    /* A retained DAT93 UI bank resolves through its immutable source even
     * while the current X body has returned to the base source. */
    CHECK(ui_banks[0].source==2 && snapshot_resolve_bank(ui_banks[0].id));
    CHECK(resolved_id==0x7000 && resolved_pixel==ui_banks[0].colors[3]);
    CHECK(snapshot_validate(ultimate,len));
    allocation_forbidden=1;snapshot_read(ultimate,len);allocation_forbidden=0;
    CHECK(x_asset_source==2);check_source_binding(0,2);check_source_binding(1,1);
    memcpy(bad,ultimate,len);
    bad[selector]=1;CHECK(!snapshot_validate(bad,len)); /* Zero cannot be X's source. */
    bad[selector]=MMX5_COOP_ARMOR_SOURCES;CHECK(!snapshot_validate(bad,len));
    CHECK(x_asset_source==2);check_source_binding(0,2);
    /* Fourth, Gaea and Falcon are now native selectable sources too. */
    for(unsigned source=3;source<MMX5_COOP_ARMOR_SOURCES;++source) {
        memcpy(bad,ultimate,len);bad[selector]=(uint8_t)source;
        CHECK(snapshot_validate(bad,len));
        allocation_forbidden=1;snapshot_read(bad,len);allocation_forbidden=0;
        CHECK(x_asset_source==source);check_source_binding(0,source);
    }
    enabled_forms[0]=1;memcpy(bad,automatic,len);
    bad[selector]=0;CHECK(!snapshot_validate(bad,len)); /* Enabled X requires DAT93. */
    CHECK(snapshot_validate(automatic,len));
    /* Restore the selected source, not whichever body binding is live now. */
    x_asset_source=0;bind_character_source(0,0);bind_character_source(1,0);
    allocation_forbidden=1;snapshot_read(automatic,len);allocation_forbidden=0;
    CHECK(x_asset_source==2);check_source_binding(0,2);check_source_binding(1,1);
    CHECK(snapshot_write(bad,len) && !memcmp(automatic,bad,len));
    free(bad);free(ultimate);free(base);
}
static void paired_scene_roundtrip(void) {
    life.status[0]=life.status[1]=MMX5_COOP_ALIVE;
    scene_owner=2;scene_phase=0;paired_victory=2;
    uint32_t len=snapshot_bytes();CHECK(len && len<1000000);
    uint8_t *saved=malloc(len),*bad=malloc(len);CHECK(saved && bad);
    CHECK(snapshot_write(saved,len));
    paired_victory=1;CHECK(snapshot_write(bad,len));
    uint32_t field=UINT32_MAX,differences=0;
    for(uint32_t i=0;i<len;++i)if(saved[i]!=bad[i]) {field=i;++differences;}
    CHECK(differences==1 && saved[field]==2 && bad[field]==1);
    unsigned before=allocations;
    bad[field]=3;CHECK(!snapshot_validate(bad,len));
    bad[field]=255;CHECK(!snapshot_validate(bad,len));
    CHECK(allocations==before && paired_victory==1 && scene_owner==2 && scene_phase==0);
    paired_victory=0;scene_owner=0;scene_phase=3;life.status[1]=MMX5_COOP_FALLEN;
    CHECK(snapshot_validate(saved,len));
    allocation_forbidden=1;snapshot_read(saved,len);allocation_forbidden=0;
    CHECK(paired_victory==2 && scene_owner==2 && scene_phase==0);
    CHECK(life.status[0]==MMX5_COOP_ALIVE && life.status[1]==MMX5_COOP_ALIVE);
    CHECK(snapshot_write(bad,len) && !memcmp(saved,bad,len));
    free(bad);free(saved);
}
static void fill_bank(SpriteBank *b,unsigned id,unsigned count,unsigned seed) {
    b->pixels=malloc(count*2);b->pixel_count=count;b->id=(uint16_t)id;
    b->source=0x80010200;b->tile=count==256?0x1010:UINT16_MAX;
    b->pixel_hash=2166136261u;
    for(unsigned i=0;i<count;++i) {b->pixels[i]=(uint16_t)(seed+i);b->pixel_hash=(b->pixel_hash^b->pixels[i])*16777619u;}
    for(unsigned i=0;i<32;++i)b->colors[i]=(uint8_t)(seed+i);
}
static void fill_sparse_bank(SpriteBank *b,unsigned id,unsigned seed) {
    fill_bank(b,id,65536,seed);memset(b->pixels,0,65536u*2u);
    for(unsigned y=0;y<32;++y)for(unsigned x=0;x<64;++x)
        b->pixels[y*256u+x]=(uint16_t)(0x8000u|((seed+x+3u*y)&0x7FFFu));
    b->pixel_hash=2166136261u;
    for(unsigned i=0;i<65536;++i)b->pixel_hash=(b->pixel_hash^b->pixels[i])*16777619u;
}
static void large_retained_cache(void) {
    for(unsigned i=0;i<bank_count;++i)free(banks[i].pixels);
    for(unsigned i=0;i<view_bank_count;++i)free(view_banks[i].pixels);
    memset(banks,0,sizeof banks);memset(view_banks,0,sizeof view_banks);
    bank_count=view_bank_count=309;
    for(unsigned i=0;i<309;++i) {
        if(i<160) {
            fill_sparse_bank(&banks[i],0x6000+i,31+i);
            fill_sparse_bank(&view_banks[i],0xA000+i,91+i);
        } else {
            fill_bank(&banks[i],0x6000+i,256,31+i);
            fill_bank(&view_banks[i],0xA000+i,256,91+i);
        }
    }
    /* This exact cache shape formerly needs42,127,760 sprite-wire bytes,
     * exceeding the SDK16MiB limit. Keep every pixel/cacheID/hash. */
    uint32_t len=snapshot_bytes();CHECK(len && len<8u*1024u*1024u);
    uint8_t *saved=malloc(len),*again=malloc(len);CHECK(saved && again);
    CHECK(snapshot_write(saved,len));
    uint16_t first_pixel=banks[0].pixels[0];
    /* Exercise the actual cache walker on a compressed retained atlas.
     * Wire header:4-bytecachecount +52-byterecord +codec1/bytecount4. */
    Mmx5SnapshotIO count={NULL,NULL,0,0,MMX5_SNAPSHOT_COUNT,1};
    unsigned one=1;snapshot_sprite_cache(&count,banks,&one,0,0x6000);
    CHECK(count.ok && count.pos>61);
    uint8_t *cache_wire=malloc(count.pos);CHECK(cache_wire);
    Mmx5SnapshotIO write={cache_wire,NULL,count.pos,0,MMX5_SNAPSHOT_WRITE,1};
    snapshot_sprite_cache(&write,banks,&one,0,0x6000);CHECK(write.ok);
    CHECK(cache_wire[56]==MMX5_COOP_PIXELS_RUNS);
    /* Count/dimension bounds must fail before any offside allocation,
     * even though a short compressed block could describe huge output. */
    const uint8_t too_many[]={1,16,0,0}; /* 4097 caches */
    unsigned allocated_before=allocations;
    Mmx5SnapshotIO invalid={NULL,too_many,sizeof too_many,0,MMX5_SNAPSHOT_VALIDATE,1};
    snapshot_sprite_cache(&invalid,banks,&one,0,0x6000);CHECK(!invalid.ok);
    CHECK(allocations==allocated_before);
    cache_wire[52]=1; /* 65537 pixels */
    invalid=(Mmx5SnapshotIO){NULL,cache_wire,count.pos,0,MMX5_SNAPSHOT_VALIDATE,1};
    snapshot_sprite_cache(&invalid,banks,&one,0,0x6000);CHECK(!invalid.ok);
    CHECK(allocations==allocated_before);cache_wire[52]=0;
    cache_wire[56]=2;
    Mmx5SnapshotIO bad={NULL,cache_wire,count.pos,0,MMX5_SNAPSHOT_VALIDATE,1};
    snapshot_sprite_cache(&bad,banks,&one,0,0x6000);CHECK(!bad.ok);
    CHECK(banks[0].pixels[0]==first_pixel);cache_wire[56]=MMX5_COOP_PIXELS_RUNS;
    cache_wire[count.pos-1]^=1; /* structurally valid changed color: pixel hash rejects */
    bad=(Mmx5SnapshotIO){NULL,cache_wire,count.pos,0,MMX5_SNAPSHOT_VALIDATE,1};
    snapshot_sprite_cache(&bad,banks,&one,0,0x6000);CHECK(!bad.ok);
    CHECK(banks[0].pixels[0]==first_pixel);free(cache_wire);
    CHECK(snapshot_validate(saved,len));
    unsigned reserved=allocations;CHECK(snapshot_validate(saved,len));CHECK(allocations==reserved);
    for(unsigned i=0;i<309;++i) {banks[i].pixels[0]^=1;view_banks[i].pixels[255]^=1;}
    allocation_forbidden=1;snapshot_read(saved,len);allocation_forbidden=0;
    for(unsigned i=0;i<309;++i) {
        CHECK(banks[i].id==0x6000+i && view_banks[i].id==0xA000+i);
        for(unsigned group=0;group<2;++group) {
            SpriteBank *b=group?&view_banks[i]:&banks[i];uint32_t hash=2166136261u;
            for(unsigned j=0;j<b->pixel_count;++j)hash=(hash^b->pixels[j])*16777619u;
            CHECK(hash==b->pixel_hash);
        }
    }
    CHECK(snapshot_write(again,len) && !memcmp(saved,again,len));
    free(again);free(saved);
}
static void duel_state_roundtrip(void) {
    StoryDuel previous=story_duel;
    uint8_t previous_story=duel_story_character;duel_story_character=1;
    memset(&story_duel,0,sizeof story_duel);
    story_duel.phase=4;story_duel.winner=1;story_duel.entrant=1;
    story_duel.character[0]=0;story_duel.character[1]=1;
    story_duel.door=0x800C6A10;story_duel.opponent=0x80092090;
    story_duel.x=6576u<<16;story_duel.y=688u<<16;
    story_duel.box=0x80076480;story_duel.damage_table=0x80076560;
    story_duel.facing[1]=0x40;story_duel.native_body[1]=0x40;
    story_duel.token[0]=0x71;story_duel.token[1]=0xE2;
    story_duel.native_body[155]=0xB7;
    story_duel.colors[0]=0x13;story_duel.colors[63]=0xF2;
    story_duel.intro_complete=story_duel.arrived=1;
    story_duel.health[0]=17;story_duel.health[1]=0;
    uint8_t wire[258],copy[258];
    Mmx5SnapshotIO io={wire,NULL,sizeof wire,0,MMX5_SNAPSHOT_WRITE,1};
    snapshot_duel(&io);CHECK(io.ok && io.pos==sizeof wire);
    memset(&story_duel,0,sizeof story_duel);
    io=(Mmx5SnapshotIO){NULL,wire,sizeof wire,0,MMX5_SNAPSHOT_VALIDATE,1};
    snapshot_duel(&io);CHECK(io.ok && !story_duel.phase);
    io=(Mmx5SnapshotIO){NULL,wire,sizeof wire,0,MMX5_SNAPSHOT_READ,1};
    snapshot_duel(&io);CHECK(io.ok && story_duel.phase==4 && story_duel.entrant==1);
    CHECK(story_duel.winner==1 && story_duel.character[0]==0 && story_duel.character[1]==1 &&
          story_duel.x==(6576u<<16) && story_duel.y==(688u<<16) &&
          story_duel.facing[1]==0x40 && story_duel.native_body[155]==0xB7 &&
          story_duel.token[0]==0x71 && story_duel.token[1]==0xE2 &&
          story_duel.colors[0]==0x13 && story_duel.colors[63]==0xF2 &&
          story_duel.intro_complete && story_duel.arrived && story_duel.health[0]==17);
    io=(Mmx5SnapshotIO){copy,NULL,sizeof copy,0,MMX5_SNAPSHOT_WRITE,1};
    snapshot_duel(&io);CHECK(io.ok && !memcmp(wire,copy,sizeof wire));
    /* Invalid admission/role/pointer wire must not mutate the live duel. */
    static const unsigned offsets[]={0,4,19,23,24,25,28,32,253,253,254,255,256,257};
    static const uint8_t values[]={0x11,0x91,0,0,5,2,0,0,0,2,2,2,33,33};
    for(unsigned i=0;i<sizeof offsets/sizeof offsets[0];++i) {
        memcpy(copy,wire,sizeof wire);copy[offsets[i]]=values[i];
        io=(Mmx5SnapshotIO){NULL,copy,sizeof copy,0,MMX5_SNAPSHOT_VALIDATE,1};
        snapshot_duel(&io);CHECK(!io.ok && story_duel.phase==4 && story_duel.entrant==1);
    }
    story_duel=previous;duel_story_character=previous_story;
}
int main(void) {
    static const unsigned records[MMX5_COOP_ARMOR_SOURCES]={88,90,93,92,94,95};
    for(unsigned i=0;i<MMX5_COOP_ARMOR_SOURCES;++i) {
        CharacterAssets *native=&source_assets[i];
        native->graphics=calloc(1,0x58000);native->ui_pixels=calloc(1024*512,2);
        native->message_bytes=17+i;native->messages=malloc(native->message_bytes);
        CHECK(native->graphics && native->ui_pixels && native->messages);
        native->loaded=1;native->graphics_bytes=0x1234+i*128;
        native->record=records[i];native->identity=0x12345678u+i;
        memset(native->graphics,(int)(21+i),0x58000);
        memset(native->messages,(int)(31+i),native->message_bytes);
        for(unsigned j=0;j<1024*512;++j)native->ui_pixels[j]=(uint16_t)((i+1)*0x1111);
        for(unsigned j=0;j<16;++j)native->hud_colors[j]=(uint16_t)(i*32+j);
    }
    for(unsigned i=0;i<2;++i) {
        assets[i].sprites=0x8F010000+i*0x58000;
        assets[i].assembly=assets[i].sprites+0x40000;assets[i].colors=assets[i].sprites+0x4C000;
        assets[i].palette=assets[i].sprites+0x50000;assets[i].menu_colors=assets[i].sprites+0x54000;
        assets[i].menu_layout=assets[i].sprites+0x54400;assets[i].message_root=0x9F180000+i*0x10000;
    }
    enabled_forms[0]=enabled_forms[1]=1;x_asset_source=2;
    bind_character_source(0,2);bind_character_source(1,1);
    arena=0x9F000000;primary_arena=0x9F080000;hud_arena=0x9F100000;
    ready=enrolled=ever_enrolled=1;counterpart=1;split_views=1;split_valid=1;
    separate_etanks=etanks_initialized=1;private_etanks[0][0]=7;private_etanks[1][1]=19;
    boss_scaling=boss_per_player=1;boss_generations[0]=17;
    CHECK(mmx5_coop_damage_bind(&boss_balance[0].damage,0x80092090,17,0x80092090));
    boss_balance[0].descriptor=1;boss_balance[0].damage.remainder=9;
    boss_balance[0].vulnerable_table=0x80075C00;boss_balance[0].window=1;
    boss_balance[0].window_phase=3;boss_balance[0].cooldown.ticks[1]=14;
    life.status[1]=MMX5_COOP_FALLEN;life.select_armed=1;scene_owner=2;scene_phase=3;
    second.body[2]=1;second.body[0x5C]=23;second.shots[4]=9;second.glow[0]=7;
    first.car[5]=5;both_door.old_min=-123;both_door.phase=2;both_door.owner=2;
    pickup_owners[0].actor=0x80098120;pickup_owners[0].seat=1;
    car_source_owner[2]=2;solid_history[1].carried=0x80;
    solid_history[2].controller_p2=1;
    landing_effects[3]=(LandingEffect){0x9F058000,0x9F098000,2};
    wide=(Mmx5WidescreenViewContext){0x800A0000,1,1,-123,0};
    bank_count=1;view_bank_count=1;
    fill_bank(&banks[0],0x6000,256,23);fill_bank(&view_banks[0],0xA000,65536,456);
    ui_bank_count=1;ui_banks[0]=(UiBank){0x60,0x7841,0x1010,0x7000,{0},2};
    for(unsigned i=0;i<16;++i)ui_banks[0].colors[i]=(uint16_t)(0x4000+i);
    menu_active=1;pause_owner=1;menu_backup_count=1;
    menu_backups[0]=(MenuBackup){320,176,704,80,malloc(704*80*2)};
    for(unsigned i=0;i<704*80;++i)menu_backups[0].pixels[i]=(uint16_t)(i*3);
    CHECK(snapshot_ready());busy=1;CHECK(!snapshot_ready());busy=0;
    source_assets[MMX5_COOP_ARMOR_SOURCES-1].loaded=0;CHECK(!snapshot_ready());
    source_assets[MMX5_COOP_ARMOR_SOURCES-1].loaded=1;
    duel_story_character=2;
    selected_zero_form=1;armor_selecting_seat=1;armor_list_seat=2;
    armor_previous_input[0]=0x40;armor_previous_input[1]=0x2000;
    uint32_t len=snapshot_bytes();CHECK(len>200000 && len<1000000);
    uint8_t *saved=malloc(len),*copy=malloc(len);CHECK(snapshot_write(saved,len));memcpy(copy,saved,len);
    CHECK(snapshot_validate(saved,len));
    unsigned before=allocations;CHECK(snapshot_validate(saved,len));CHECK(allocations==before);
    CHECK(!snapshot_validate(saved,len-1));CHECK(!snapshot_validate(saved,len+1));
    saved[0]^=1;CHECK(!snapshot_validate(saved,len));saved[0]^=1;
    CHECK(saved[4]==13 && !saved[5] && !saved[6] && !saved[7]);
    saved[4]=12;CHECK(!snapshot_validate(saved,len));saved[4]=13;
    enabled_forms[0]=0;CHECK(!snapshot_validate(saved,len));enabled_forms[0]=1;
    enabled_forms[1]=0;CHECK(snapshot_validate(saved,len));enabled_forms[1]=1;
    for(unsigned i=0;i<MMX5_COOP_ARMOR_SOURCES;++i) {
        source_assets[i].identity^=1;CHECK(!snapshot_validate(saved,len));source_assets[i].identity^=1;
        ++source_assets[i].record;CHECK(!snapshot_validate(saved,len));--source_assets[i].record;
    }
    ++source_assets[2].graphics_bytes;CHECK(!snapshot_validate(saved,len));--source_assets[2].graphics_bytes;
    ++source_assets[2].message_bytes;CHECK(!snapshot_validate(saved,len));--source_assets[2].message_bytes;
    boss_per_player=0;CHECK(!snapshot_validate(saved,len));boss_per_player=1;
    boss_scaling=0;CHECK(!snapshot_validate(saved,len));boss_scaling=1;
    balance_current=1;CHECK(!snapshot_ready());balance_current=0;
    balance_attempted=1;CHECK(!snapshot_ready());balance_attempted=0;
    balance_pending_actor=0x80092090;CHECK(!snapshot_ready());balance_pending_actor=0;
    split_views=0;CHECK(!snapshot_validate(saved,len));split_views=1;
    CHECK(second.body[0x5C]==23 && menu_backups[0].pixels[123]==369 && private_etanks[0][0]==7);
    CHECK(snapshot_validate(saved,len));
    /* Load an older, larger cache into a smaller live timeline and vice versa. */
    second.body[0x5C]=4;private_etanks[0][0]=0;scene_owner=0;pause_owner=-1;
    solid_history[2].controller_p2=0;
    memset(boss_balance,0,sizeof boss_balance);boss_generations[0]=0;
    memset(&landing_effects[3],0,sizeof landing_effects[3]);
    x_asset_source=0;bind_character_source(0,0);bind_character_source(1,0);
    banks[0].pixels[0]=999;view_banks[0].pixels[32767]=0;menu_backups[0].pixels[123]=0;
    duel_story_character=0;
    selected_zero_form=armor_selecting_seat=armor_list_seat=0;
    armor_previous_input[0]=armor_previous_input[1]=0;
    allocation_forbidden=1;snapshot_read(saved,len);allocation_forbidden=0;
    CHECK(duel_story_character==2);
    CHECK(selected_zero_form==1 && armor_selecting_seat==1 && armor_list_seat==2 &&
          armor_previous_input[0]==0x40 && armor_previous_input[1]==0x2000);
    CHECK(second.body[0x5C]==23 && private_etanks[0][0]==7 && scene_owner==2 && pause_owner==1);
    CHECK(banks[0].pixels[0]==23 && view_banks[0].pixels[32767]==(uint16_t)(456+32767));
    CHECK(menu_backups[0].pixels[123]==369 && both_door.old_min==-123 && solid_history[1].carried==0x80);
    CHECK(solid_history[2].controller_p2==1);
    CHECK(x_asset_source==2);check_source_binding(0,2);check_source_binding(1,1);
    CHECK(boss_generations[0]==17 && boss_balance[0].damage.remainder==9 &&
          boss_balance[0].cooldown.ticks[1]==14 && boss_balance[0].window==1);
    CHECK(landing_effects[3].owner==2 && landing_effects[3].sprites==0x9F058000 &&
          landing_effects[3].assembly==0x9F098000);
    CHECK(wide.ui_x_delta==-123 && ui_banks[0].colors[0]==0x4000 && ui_banks[0].source==2);
    CHECK(snapshot_write(copy,len));CHECK(!memcmp(saved,copy,len));
    /* Saved length/count is independent of today's warm cache. */
    free(view_banks[0].pixels);view_banks[0].pixels=NULL;view_bank_count=0;
    CHECK(snapshot_validate(saved,len));allocation_forbidden=1;snapshot_read(saved,len);allocation_forbidden=0;
    CHECK(view_bank_count==1 && view_banks[0].pixels[0]==456);
    Mmx5SnapshotIO balance_count={NULL,NULL,0,0,MMX5_SNAPSHOT_COUNT,1};
    snapshot_balance(&balance_count);CHECK(balance_count.ok);
    uint8_t *balance_wire=malloc(balance_count.pos);CHECK(balance_wire);
    Mmx5SnapshotIO balance_write={balance_wire,NULL,balance_count.pos,0,MMX5_SNAPSHOT_WRITE,1};
    snapshot_balance(&balance_write);CHECK(balance_write.ok);
    /* generation4 + actor4 + actor-generation4 + bank4 + serial4 =20. */
    balance_wire[20]=10;
    Mmx5SnapshotIO balance_bad={NULL,balance_wire,balance_count.pos,0,MMX5_SNAPSHOT_VALIDATE,1};
    snapshot_balance(&balance_bad);CHECK(!balance_bad.ok && boss_balance[0].damage.remainder==9);
    balance_wire[20]=9;balance_wire[29]=10; /* descriptor beyond audited9. */
    balance_bad=(Mmx5SnapshotIO){NULL,balance_wire,balance_count.pos,0,MMX5_SNAPSHOT_VALIDATE,1};
    snapshot_balance(&balance_bad);CHECK(!balance_bad.ok && boss_balance[0].descriptor==1);
    free(balance_wire);
    form_source_roundtrip(saved,len);
    paired_scene_roundtrip();
    duel_state_roundtrip();
    large_retained_cache();
    puts("X5 schema13 snapshot roundtrip, unit choice/input history, duel role/pointer rejection, paired state2/range rejection, six form/source/UI bindings and allocation-free read passed");return 0;
}
