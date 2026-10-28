/* Title-owned host state. Included after the private co-op implementation so
 * there is one authoritative state layout, with no public mutable globals. */
#include "mmx5_coop_pixel_codec.h"
/* Existing structural limits bound offside decoded pixels to:
 * 2*4096*65536*2 + 32*1024*512*2 = 1,107,296,256 bytes maximum.
 * Do not confuse the SDK16MiB wire limit with this decoded bound: a real
 * captured cache uses42,095,616 pixel bytes. Each block is validated before
 * its bounded allocation; allocation failure rejects without live mutation. */
static SpriteBank restore_banks[2][4096];
static uint32_t restore_bank_capacity[2][4096];
static MenuBackup restore_menus[32];
static uint32_t restore_menu_capacity[32];

static void snapshot_pixels(Mmx5SnapshotIO *s,const uint16_t *live,uint16_t **prepared,
                            uint32_t *capacity,uint32_t count) {
    if(!s->ok || count>MMX5_COOP_PIXELS_MAX_COUNT) {s->ok=0;return;}
    uint8_t encoding=MMX5_COOP_PIXELS_RAW;uint32_t bytes=0;
    if((s->mode==MMX5_SNAPSHOT_COUNT || s->mode==MMX5_SNAPSHOT_WRITE) &&
       !mmx5_coop_pixels_measure(live,count,&encoding,&bytes)) {s->ok=0;return;}
    encoding=(uint8_t)mmx5_snapshot_integer(s,encoding,1,MMX5_COOP_PIXELS_RUNS);
    bytes=mmx5_snapshot_integer(s,bytes,4,count*2u);
    if(!s->ok || bytes>UINT32_MAX-s->pos ||
       (s->mode!=MMX5_SNAPSHOT_COUNT && (s->pos>s->len || bytes>s->len-s->pos))) {
        s->ok=0;return;
    }
    if(s->mode==MMX5_SNAPSHOT_WRITE) {
        if(!mmx5_coop_pixels_encode(s->out+s->pos,bytes,live,count,encoding))s->ok=0;
    } else if(s->mode==MMX5_SNAPSHOT_VALIDATE) {
        /* Reject lengths/runs before allocating or writing offside pixels.
         * Every provider validates before restore; the read below allocates
         * and decodes nothing, retaining this exact validated pixel buffer. */
        if(!mmx5_coop_pixels_decode(s->in+s->pos,bytes,count,encoding,NULL)) {s->ok=0;return;}
        if(*capacity<count) {
            uint16_t *p=realloc(*prepared,(size_t)count*2u);
            if(!p) {s->ok=0;return;}
            *prepared=p;*capacity=count;
        }
        if(!mmx5_coop_pixels_decode(s->in+s->pos,bytes,count,encoding,*prepared))s->ok=0;
    } else if(s->mode==MMX5_SNAPSHOT_READ && count &&
              (!*prepared || *capacity<count)) {s->ok=0;return;}
    s->pos+=bytes;
}
static void snapshot_sprite_cache(Mmx5SnapshotIO *s,SpriteBank *cache,unsigned *used,unsigned group,unsigned first_id) {
    uint32_t count=mmx5_snapshot_integer(s,*used,4,4096);
    if(!s->ok)return;
    for(uint32_t i=0;i<count && s->ok;++i) {
        SpriteBank b=cache[i];
        /* Locals consume fields during validation without touching gameplay. */
#define SPRITE_FIELD(field,n,max) b.field=mmx5_snapshot_integer(s,b.field,n,max)
        SPRITE_FIELD(source,4,UINT32_MAX);SPRITE_FIELD(generation,4,UINT32_MAX);
        SPRITE_FIELD(id,2,UINT16_MAX);SPRITE_FIELD(tile,2,UINT16_MAX);
        if(s->mode==MMX5_SNAPSHOT_VALIDATE) {
            if(s->pos>s->len || sizeof b.colors>s->len-s->pos) {s->ok=0;return;}
            memcpy(b.colors,s->in+s->pos,sizeof b.colors);
        }
        mmx5_snapshot_bytes(s,b.colors,sizeof b.colors);
        SPRITE_FIELD(pixel_hash,4,UINT32_MAX);SPRITE_FIELD(pixel_count,4,65536);
#undef SPRITE_FIELD
        if((b.pixel_count!=0 && b.pixel_count!=256 && b.pixel_count!=65536) ||
           (b.pixel_count && b.id!=first_id+i) || (!b.pixel_count && b.id)) {s->ok=0;return;}
        snapshot_pixels(s,cache[i].pixels,&restore_banks[group][i].pixels,
                        &restore_bank_capacity[group][i],b.pixel_count);
        if(s->mode==MMX5_SNAPSHOT_VALIDATE && s->ok && b.pixel_count) {
            uint32_t hash=2166136261u;
            for(uint32_t j=0;j<b.pixel_count;++j)hash=(hash^restore_banks[group][i].pixels[j])*16777619u;
            if(hash!=b.pixel_hash)s->ok=0;
        }
        if(s->mode==MMX5_SNAPSHOT_READ) {
            free(cache[i].pixels);b.pixels=restore_banks[group][i].pixels;
            cache[i]=b;restore_banks[group][i].pixels=NULL;restore_bank_capacity[group][i]=0;
        }
    }
    if(s->mode==MMX5_SNAPSHOT_READ) {
        for(unsigned i=count;i<*used;++i) {free(cache[i].pixels);memset(&cache[i],0,sizeof cache[i]);}
        *used=count;
    }
}
static void snapshot_ui_cache(Mmx5SnapshotIO *s,UiBank *cache,unsigned *used,unsigned first_id) {
    uint32_t count=mmx5_snapshot_integer(s,*used,4,4096);
    if(!s->ok)return;
    for(uint32_t i=0;i<count && s->ok;++i) {
        UiBank b=cache[i];
#define UI_FIELD(field,n,max) b.field=mmx5_snapshot_integer(s,b.field,n,max)
        UI_FIELD(page,2,UINT16_MAX);UI_FIELD(clut,2,32767);UI_FIELD(uv,2,UINT16_MAX);
        UI_FIELD(id,2,UINT16_MAX);UI_FIELD(source,1,MMX5_COOP_ARMOR_SOURCES-1);
#undef UI_FIELD
        if(b.id!=first_id+i || (b.page&0x180))s->ok=0;
        for(unsigned j=0;j<16;++j)b.colors[j]=(uint16_t)mmx5_snapshot_integer(s,b.colors[j],2,UINT16_MAX);
        if(s->mode==MMX5_SNAPSHOT_READ)cache[i]=b;
    }
    if(s->mode==MMX5_SNAPSHOT_READ)*used=count;
}
static void snapshot_menu(Mmx5SnapshotIO *s) {
    uint32_t count=mmx5_snapshot_integer(s,menu_backup_count,1,32);
    if(!s->ok)return;
    for(unsigned i=0;i<count && s->ok;++i) {
        MenuBackup b=menu_backups[i];
        b.x=mmx5_snapshot_integer(s,b.x,2,1023);b.y=mmx5_snapshot_integer(s,b.y,2,511);
        b.w=mmx5_snapshot_integer(s,b.w,2,1024);b.h=mmx5_snapshot_integer(s,b.h,2,512);
        if(!b.w || !b.h || b.w>1024-b.x || b.h>512-b.y) {s->ok=0;return;}
        snapshot_pixels(s,menu_backups[i].pixels,&restore_menus[i].pixels,&restore_menu_capacity[i],b.w*b.h);
        if(s->mode==MMX5_SNAPSHOT_READ) {
            free(menu_backups[i].pixels);b.pixels=restore_menus[i].pixels;
            menu_backups[i]=b;restore_menus[i].pixels=NULL;restore_menu_capacity[i]=0;
        }
    }
    if(s->mode==MMX5_SNAPSHOT_READ) {
        for(unsigned i=count;i<menu_backup_count;++i) {free(menu_backups[i].pixels);menu_backups[i].pixels=NULL;}
        menu_backup_count=count;
    }
}
static void snapshot_balance(Mmx5SnapshotIO *io) {
    for(unsigned i=0;i<48 && io->ok;++i) {
        BossBalanceState s=boss_balance[i];uint32_t generation=boss_generations[i];
#define BFIELD(field,n,max) s.field=mmx5_snapshot_integer(io,s.field,n,max)
        generation=mmx5_snapshot_integer(io,generation,4,UINT32_MAX);
        BFIELD(damage.actor,4,UINT32_MAX);BFIELD(damage.generation,4,UINT32_MAX);
        BFIELD(damage.bank,4,UINT32_MAX);BFIELD(damage.serial,4,UINT32_MAX);
        BFIELD(damage.remainder,1,9);BFIELD(controller,4,UINT32_MAX);
        BFIELD(vulnerable_table,4,UINT32_MAX);BFIELD(descriptor,1,9);
        BFIELD(window,1,1);BFIELD(window_phase,1,UINT8_MAX);
        BFIELD(cooldown.ticks[0],1,15);BFIELD(cooldown.ticks[1],1,15);
#undef BFIELD
        if(!io->ok)return;
        if(!s.descriptor) {
            if(s.damage.actor || s.damage.generation || s.damage.bank || s.damage.serial ||
               s.damage.remainder || s.controller || s.vulnerable_table || s.window ||
               s.window_phase || s.cooldown.ticks[0] || s.cooldown.ticks[1])io->ok=0;
        }else {
            const BossBalanceDescriptor *d=&balance_descriptors[s.descriptor-1u];
            if(!boss_scaling || s.damage.actor!=0x80092090u+i*0x9Cu ||
               s.damage.generation!=generation ||
               (d->controller?!balance_ram(s.controller):s.controller!=0) ||
               s.damage.bank!=(d->controller?s.controller:s.damage.actor) ||
               (s.vulnerable_table && (!balance_table(s.vulnerable_table) ||
                                      s.vulnerable_table==BOSS_REJECTION_TABLE)) ||
               (s.window && (!boss_per_player || !s.vulnerable_table)) ||
               (!boss_per_player && (s.cooldown.ticks[0] || s.cooldown.ticks[1])))io->ok=0;
        }
        if(io->ok && io->mode==MMX5_SNAPSHOT_READ) {boss_balance[i]=s;boss_generations[i]=generation;}
    }
}
static int snapshot_ready(void) {
    const unsigned calls[]={busy,projected,stage_call,player_call,world_owner,render_call,camera_call,
        camera_update_call,split_camera_call,split_scan_call,split_supplemental,split_bounds_call,
        local_view_call,spatial_call,personal_effect_call,personal_visual_call,area_call,
        ai_pool_call,ai_call,ai_suspended,effect_allocate_call,effect_init_call,
        hud_call,hud_icon_call,upload_call,allocation_call,solid_call,contact_call,pickup_call,
        scene_call,door_call,door_traversal_call,door_terrain_call,ride_call,ride_spawn_call,
        balance_current,balance_call,balance_contact_call,balance_draw_call,
        balance_pending_actor,balance_pending_function,balance_prepared,balance_attempted,
        duel_contact_call,duel_native_contact,duel_native_injury,duel_render_art!=NULL,duel_opponent_call,duel_text_call,
        personal_menu_call,personal_rendering,personal_shared_call,armor_ui_call};
    for(unsigned i=0;i<sizeof calls/sizeof calls[0];++i)if(calls[i])return 0;
    return !psx_mod_local_view_scope() && source_assets[0].loaded && source_assets[1].loaded &&
        source_assets[MMX5_COOP_ARMOR_SOURCES-1].loaded && mmx5_coop_audio_snapshot_ready();
}
static void snapshot_duel(Mmx5SnapshotIO *s) {
    StoryDuel d=story_duel;
    unsigned story=duel_story_character;
#define DUEL_FIELD(field,n,max) d.field=mmx5_snapshot_integer(s,d.field,n,max)
    DUEL_FIELD(door,4,UINT32_MAX);DUEL_FIELD(opponent,4,UINT32_MAX);
    DUEL_FIELD(x,4,UINT32_MAX);DUEL_FIELD(y,4,UINT32_MAX);
    DUEL_FIELD(box,4,UINT32_MAX);DUEL_FIELD(damage_table,4,UINT32_MAX);
    DUEL_FIELD(phase,1,4);DUEL_FIELD(entrant,1,1);
    for(unsigned i=0;i<2;++i) {
        DUEL_FIELD(character[i],1,1);DUEL_FIELD(facing[i],1,255);
    }
    for(unsigned i=0;i<2;++i)DUEL_FIELD(token[i],1,255);
    DUEL_FIELD(winner,1,2);
    mmx5_snapshot_bytes(s,d.native_body,sizeof d.native_body);
    mmx5_snapshot_bytes(s,d.colors,sizeof d.colors);
    story=mmx5_snapshot_integer(s,story,1,2);
    DUEL_FIELD(intro_complete,1,1);DUEL_FIELD(arrived,1,1);
    for(unsigned i=0;i<2;++i)DUEL_FIELD(health[i],1,32);
#undef DUEL_FIELD
    if((d.phase==4)!=(d.winner!=0))s->ok=0;
    if(d.phase>=3 && (!d.intro_complete || !d.arrived))s->ok=0;
    if(d.phase==4 && (!d.winner || d.winner>2 ||
       story!=d.character[d.winner-1u]+1u))s->ok=0;
    if(d.phase) {
        if(d.door<0x800C6A10u || d.door>=0x800C9310u ||
           (d.door-0x800C6A10u)%0xA4u || d.character[0]==d.character[1])s->ok=0;
        if(d.opponent && (d.opponent<0x80092090u || d.opponent>=0x80093DD0u ||
           (d.opponent-0x80092090u)%0x9Cu))s->ok=0;
        if(d.phase>=2 && (d.opponent<0x80092090u || d.opponent>=0x80093DD0u ||
           (d.opponent-0x80092090u)%0x9Cu || d.box<0x80010000u || d.box>0x801FFFFCu ||
           d.damage_table<0x80010000u || d.damage_table>0x801FFF80u))s->ok=0;
    }
    if(s->ok && s->mode==MMX5_SNAPSHOT_READ) {
        story_duel=d;duel_story_character=(uint8_t)story;
    }
}
static void snapshot_walk(Mmx5SnapshotIO *s) {
    mmx5_snapshot_fixed(s,0x42503558u); /* X5PB */
    mmx5_snapshot_fixed(s,15); /* Independent native Ride Armor placement. */
    mmx5_snapshot_fixed(s,split_views);mmx5_snapshot_fixed(s,local_split);
    mmx5_snapshot_fixed(s,both_door_players);mmx5_snapshot_fixed(s,keep_zero);
    mmx5_snapshot_fixed(s,separate_etanks);
    mmx5_snapshot_fixed(s,boss_scaling);mmx5_snapshot_fixed(s,boss_per_player);
    mmx5_snapshot_fixed(s,mmx5_coop_forms_enabled(0));
    /* Zero's form is a mutable unit choice below, not a boot preference. */
    mmx5_snapshot_preference(s,hud_side_by_side);
#ifdef MMX5_COOP_TEST_CHEATS
    mmx5_snapshot_fixed(s,1);
    MMX5_SNAPSHOT_U32(s,test_chord);MMX5_SNAPSHOT_U32(s,test_god_mask);
    MMX5_SNAPSHOT_U32(s,test_contact_mask);MMX5_SNAPSHOT_U32(s,test_commands);
#else
    mmx5_snapshot_fixed(s,0);
#endif
#ifndef PSX_NO_DEBUG_TOOLS
    mmx5_snapshot_fixed(s,1);
    MMX5_SNAPSHOT_BOOL(s,fixture_location_pending);MMX5_SNAPSHOT_FIELD(s,fixture_stage,1,22);
    MMX5_SNAPSHOT_FIELD(s,fixture_section,1,1);MMX5_SNAPSHOT_FIELD(s,fixture_checkpoint,1,127);
#else
    mmx5_snapshot_fixed(s,0);
#endif
    mmx5_snapshot_fixed(s,arena);mmx5_snapshot_fixed(s,primary_arena);
    mmx5_snapshot_fixed(s,hud_arena);mmx5_snapshot_fixed(s,diagnostic);
    mmx5_snapshot_fixed(s,extra_ride_record);
    uint8_t ride_record[8]={0};
    if(extra_ride_record)capture(extra_ride_record,ride_record,sizeof ride_record);
    mmx5_snapshot_bytes(s,ride_record,sizeof ride_record);
    if(s->ok && s->mode==MMX5_SNAPSHOT_READ)
        project(extra_ride_record,ride_record,sizeof ride_record);
    for(unsigned c=0;c<2;++c) {
        CharacterAssets *a=&assets[c];
        mmx5_snapshot_fixed(s,a->sprites);mmx5_snapshot_fixed(s,a->assembly);
        mmx5_snapshot_fixed(s,a->colors);mmx5_snapshot_fixed(s,a->palette);
        mmx5_snapshot_fixed(s,a->menu_colors);mmx5_snapshot_fixed(s,a->menu_layout);
        mmx5_snapshot_fixed(s,a->message_root);
        mmx5_snapshot_fixed(s,car_mirror[c]);
    }
    for(unsigned c=0;c<MMX5_COOP_ARMOR_SOURCES;++c) {
        const CharacterAssets *a=&source_assets[c];
        mmx5_snapshot_fixed(s,a->record);mmx5_snapshot_fixed(s,a->identity);
        mmx5_snapshot_fixed(s,(uint32_t)a->graphics_bytes);
        mmx5_snapshot_fixed(s,(uint32_t)a->message_bytes);
    }
    uint32_t source=mmx5_snapshot_integer(s,x_asset_source,1,MMX5_COOP_ARMOR_SOURCES-1);
    MMX5_SNAPSHOT_FIELD(s,selected_x_armor,1,4);
    MMX5_SNAPSHOT_BOOL(s,selected_zero_form);
    MMX5_SNAPSHOT_BOOL(s,armor_selection_seen);
    MMX5_SNAPSHOT_FIELD(s,armor_selecting_seat,1,2);
    MMX5_SNAPSHOT_FIELD(s,armor_list_seat,1,3);
    for(unsigned seat=0;seat<2;++seat)MMX5_SNAPSHOT_U16(s,armor_previous_input[seat]);
    MMX5_SNAPSHOT_BOOL(s,armor_roster_active);
    MMX5_SNAPSHOT_FIELD(s,armor_native_count,1,6);
    mmx5_snapshot_bytes(s,armor_native_roster,sizeof armor_native_roster);
    if(source==1 || (mmx5_coop_forms_enabled(0) && source!=2))s->ok=0;
    if(s->ok && s->mode==MMX5_SNAPSHOT_READ) {
        x_asset_source=source;bind_character_source(0,source);bind_character_source(1,1);
    }
#define BYTES(field) mmx5_snapshot_bytes(s,(field),sizeof(field))
#define BOOL(field) MMX5_SNAPSHOT_BOOL(s,field)
#define U32(field) MMX5_SNAPSHOT_U32(s,field)
    BYTES(first.body);BYTES(first.shots);BYTES(first.glow);BYTES(first.trails);BYTES(first.car);
    BYTES(second.body);BYTES(second.shots);BYTES(second.glow);BYTES(second.trails);BYTES(second.car);
    BYTES(saved_play);BYTES(saved_camera);
    for(unsigned i=0;i<6;++i)U32(resources[i]);
    BOOL(ready);BOOL(enrolled);BOOL(failed);BOOL(counterpart);BOOL(etanks_initialized);
    for(unsigned c=0;c<2;++c)for(unsigned i=0;i<2;++i)MMX5_SNAPSHOT_FIELD(s,private_etanks[c][i],1,32);
    BOOL(split_valid);MMX5_SNAPSHOT_FIELD(s,split_owner,1,2);U32(split_table);
    BYTES(split_cameras);BYTES(split_visits);
    MMX5_SNAPSHOT_FIELD(s,ai_owner,1,1);U32(ai_pool);U32(ai_pending);U32(ai_function);
    MMX5_SNAPSHOT_U8(s,ai_first_freeze);U32(area_actor);U32(area_function);
    U32(dinorex_fire.actor);MMX5_SNAPSHOT_FIELD(s,dinorex_fire.owner,1,2);BYTES(dinorex_fire.camera);
    for(unsigned c=0;c<2;++c)U32(personal_visual_ticks[c]);
    for(unsigned i=0;i<96;++i)MMX5_SNAPSHOT_FIELD(s,personal_effect_owner[i],1,2);
    for(unsigned i=0;i<128;++i) {
        DeathOrigin *d=&death_origins[i];
        U32(d->x);U32(d->y);U32(d->assembly);MMX5_SNAPSHOT_U8(s,d->layer);BOOL(d->valid);
        LandingEffect *landing=&landing_effects[i];
        U32(landing->sprites);U32(landing->assembly);MMX5_SNAPSHOT_FIELD(s,landing->owner,1,2);
    }
    U32(fixture_sequence);U32(primary_packets);U32(world_packets);U32(ticks);U32(world_ticks);U32(asset_generation);
    MMX5_SNAPSHOT_U16(s,previous_input);BYTES(returning_body);U32(join_y);
    MMX5_SNAPSHOT_FIELD(s,join_phase,1,4);
    for(unsigned i=0;i<2;++i)MMX5_SNAPSHOT_FIELD(s,life.status[i],1,MMX5_COOP_UNAVAILABLE);
    U32(life.select_frames);BOOL(life.select_armed);BOOL(life.select_previous);
    MMX5_SNAPSHOT_FIELD(s,life.wipe,1,2);
    MMX5_SNAPSHOT_FIELD(s,hud_fade,1,128);MMX5_SNAPSHOT_FIELD(s,hud_seat,1,1);
    BOOL(menu_active);BOOL(personal_active);BOOL(personal_initialized);
    MMX5_SNAPSHOT_FIELD(s,personal_eligible,1,3);
    for(unsigned seat=0;seat<2;++seat) {
        BYTES(personal_menus[seat].native);MMX5_SNAPSHOT_U8(s,personal_menus[seat].original_weapon);
        MMX5_SNAPSHOT_U16(s,personal_menu_previous[seat]);
    }
    uint32_t owner=mmx5_snapshot_integer(s,(uint32_t)(pause_owner+1),1,2);
    if(s->mode==MMX5_SNAPSHOT_READ)pause_owner=(int)owner-1;
    BOOL(p2_start_previous);BOOL(ever_enrolled);
    MMX5_SNAPSHOT_FIELD(s,last_stage,1,22);MMX5_SNAPSHOT_FIELD(s,last_section,1,1);
    BOOL(section_pending);BOOL(stage_departed);
    MMX5_SNAPSHOT_FIELD(s,hit_count,2,256);
    /* Fixed capacity avoids reading a count into live state during validation. */
    for(unsigned i=0;i<256;++i) {U32(hit_history[i].actor);BYTES(hit_history[i].token);}
    for(unsigned i=0;i<64;++i) {
        MMX5_SNAPSHOT_U8(s,solid_history[i].sides);MMX5_SNAPSHOT_U8(s,solid_history[i].carried);
        MMX5_SNAPSHOT_U8(s,solid_history[i].car_sides);MMX5_SNAPSHOT_U8(s,solid_history[i].car_carried);
        MMX5_SNAPSHOT_BOOL(s,solid_history[i].controller_p2);
    }
    for(unsigned i=0;i<128;++i) {U32(pickup_owners[i].actor);MMX5_SNAPSHOT_FIELD(s,pickup_owners[i].seat,1,1);}
    MMX5_SNAPSHOT_FIELD(s,scene_owner,1,2);MMX5_SNAPSHOT_FIELD(s,scene_phase,1,6);
    MMX5_SNAPSHOT_FIELD(s,paired_victory,1,2);BYTES(scene_body);U32(scene_y);
    U32(both_door.actor);MMX5_SNAPSHOT_U16(s,both_door.old_min);MMX5_SNAPSHOT_U16(s,both_door.old_max);
    MMX5_SNAPSHOT_U16(s,both_door.target_min);MMX5_SNAPSHOT_U16(s,both_door.target_max);MMX5_SNAPSHOT_U16(s,both_door.right);
    MMX5_SNAPSHOT_FIELD(s,both_door.stage,1,22);MMX5_SNAPSHOT_FIELD(s,both_door.section,1,1);
    MMX5_SNAPSHOT_FIELD(s,both_door.phase,1,4);MMX5_SNAPSHOT_FIELD(s,both_door.owner,1,2);
    BOOL(both_door.owns_lock);BOOL(door_wait);
    snapshot_duel(s);
    for(unsigned i=0;i<272;++i)MMX5_SNAPSHOT_FIELD(s,car_source_owner[i],1,2);
    Mmx5WidescreenViewContext view;mmx5_widescreen_view_save(&view);
    U32(view.ui_begin);BOOL(view.ui_pending);BOOL(view.ui_screen);U32(view.ui_x_delta);BOOL(view.supplemental_scan);
    if(s->mode==MMX5_SNAPSHOT_READ)mmx5_widescreen_view_restore(&view);
    snapshot_balance(s);
    mmx5_coop_audio_snapshot(s);
    snapshot_sprite_cache(s,banks,&bank_count,0,0x6000);
    snapshot_sprite_cache(s,view_banks,&view_bank_count,1,0xA000);
    snapshot_ui_cache(s,ui_banks,&ui_bank_count,0x7000);
    snapshot_ui_cache(s,view_ui_banks,&view_ui_bank_count,0xB000);
    snapshot_menu(s);
#undef U32
#undef BOOL
#undef BYTES
}
static uint32_t snapshot_bytes(void) {
    Mmx5SnapshotIO s={NULL,NULL,0,0,MMX5_SNAPSHOT_COUNT,1};snapshot_walk(&s);
    return s.ok?s.pos:UINT32_MAX;
}
static int snapshot_write(uint8_t *out,uint32_t len) {
    Mmx5SnapshotIO s={out,NULL,len,0,MMX5_SNAPSHOT_WRITE,1};snapshot_walk(&s);return s.ok && s.pos==len;
}
static int snapshot_validate(const uint8_t *data,uint32_t len) {
    if(len>16u*1024u*1024u || !source_assets[0].loaded || !source_assets[1].loaded ||
       !source_assets[MMX5_COOP_ARMOR_SOURCES-1].loaded)return 0;
    Mmx5SnapshotIO s={NULL,data,len,0,MMX5_SNAPSHOT_VALIDATE,1};snapshot_walk(&s);return s.ok && s.pos==len;
}
static void snapshot_read(const uint8_t *data,uint32_t len) {
    Mmx5SnapshotIO s={NULL,data,len,0,MMX5_SNAPSHOT_READ,1};snapshot_walk(&s);
}
static int snapshot_resolve_bank(uint16_t id) {
    SpriteBank *b=NULL;UiBank *u=NULL;
    if(id>=0x6000 && id<0x6000+bank_count)b=&banks[id-0x6000];
    else if(id>=0xA000 && id<0xA000+view_bank_count)b=&view_banks[id-0xA000];
    else if(id>=0x7000 && id<0x7000+ui_bank_count)u=&ui_banks[id-0x7000];
    else if(id>=0xB000 && id<0xB000+view_ui_bank_count)u=&view_ui_banks[id-0xB000];
    if(b && b->pixels && b->id==id) {
        unsigned edge=b->pixel_count==256?16:256;
        return psx_mod_define_texture_bank(id,edge,edge,b->pixels);
    }
    if(u && u->id==id && u->source<3) {
        uint16_t pixels[256];unsigned bx=(u->page&15)*64,by=(u->page&16)*16;
        for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x) {
            unsigned v=((u->uv>>8)+y)&255,w=((u->uv&255)+x)&255;
            uint16_t p=source_assets[u->source].ui_pixels[(by+v)*1024+bx+w/4];
            pixels[y*16+x]=u->colors[(p>>((w&3)*4))&15];
        }
        return psx_mod_define_texture_bank(id,16,16,pixels);
    }
    return 0;
}
static void snapshot_restored(void) {
    (void)psx_mod_forget_texture_bank_range(0x6000,4096);
    (void)psx_mod_forget_texture_bank_range(0x7000,4096);
    (void)psx_mod_forget_texture_bank_range(0xA000,4096);
    (void)psx_mod_forget_texture_bank_range(0xB000,4096);
    psx_mod_set_texture_bank_resolver(snapshot_resolve_bank);
    mmx5_coop_audio_restored();
}
