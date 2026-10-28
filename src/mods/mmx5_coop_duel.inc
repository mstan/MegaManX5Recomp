/* Story-duel entrance and presentation ownership. PlayerContext remains the
 * sole body/weapon/menu state for each seat, including the NPC-side player. */
static int duel_scope(void) {
    return story_duel.phase && psx_mod_read_byte(PLAY)==10 &&
        psx_mod_read_byte(PLAY+12)==18 && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==0x20u;
}
static int duel_participant(unsigned seat) {
    return life.status[seat]==MMX5_COOP_ALIVE || life.status[seat]==MMX5_COOP_DYING ||
        life.status[seat]==MMX5_COOP_FALLEN;
}
static int duel_inner_door(uint32_t actor) {
    /* Original BIN28 placement9, FCF C4: class8/sub8 at6384,768.
     * Placement8/sub7 is the preceding door and must retain ordinary co-op. */
    return !story_duel.phase && psx_mod_read_byte(PLAY)==10 &&
        psx_mod_read_byte(PLAY+12)==18 && !psx_mod_read_byte(PLAY+13) &&
        psx_mod_read_word(0x800EE970u)==0x20u &&
        actor>=0x800C6A10u && actor<0x800C9310u && !((actor-0x800C6A10u)%0xA4u) &&
        psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==8 &&
        psx_mod_read_byte(actor+2)==8 && psx_mod_read_half(actor+10)==6384 &&
        psx_mod_read_half(actor+14)==768 &&
        duel_participant(0) && duel_participant(1) &&
        (life.status[0]==MMX5_COOP_ALIVE || life.status[1]==MMX5_COOP_ALIVE);
}
static void duel_revive(CPUState *cpu,unsigned seat) {
    if(life.status[seat]!=MMX5_COOP_DYING && life.status[seat]!=MMX5_COOP_FALLEN)return;
    uint8_t play[sizeof saved_play],camera[sizeof saved_camera],inventory[0x158];
    uint8_t projection_play[sizeof saved_play],projection_camera[sizeof saved_camera];
    uint32_t caller_resources[6],projection_resources[6];
    capture(PLAY,play,sizeof play);capture(CAMERA,camera,sizeof camera);
    memcpy(projection_play,saved_play,sizeof saved_play);memcpy(projection_camera,saved_camera,sizeof saved_camera);
    memcpy(projection_resources,resources,sizeof resources);
    for(unsigned i=0;i<6;++i)caller_resources[i]=psx_mod_read_word(SCRATCH+resource_offsets[i]);
    unsigned was=projected,old_busy=busy;busy=1;both_project(seat);
    capture(PLAYER,inventory,sizeof inventory);clear_buster_effects(seat);
    clear_personal_drawables(seat);
    for(unsigned i=0;i<sizeof first.shots;++i)psx_mod_write_byte(SHOTS+i,0);
    /* Like ordinary enrollment, native3BAC8 expects a fresh body. It does
     * not clear fatal7A, captureCC, sceneD0/D4 or mountedD5 by itself. */
    for(unsigned i=0;i<sizeof inventory;++i)psx_mod_write_byte(PLAYER+i,0);
    /* This encounter explicitly revives a previously fallen participant.
     * Use the ordinary native body/parts/movement initialization, preserving
     * acquired moves and selected weapon/ammo. It does not spend/refund a
     * shared retry or revive anyone at an ordinary door. */
    psx_mod_write_byte(PLAY+0x1E,0);guest(cpu,0x8003BAC8u,0,0);
    guest(cpu,0x8003D5E0u,PLAYER,0);guest(cpu,0x8003B65Cu,PLAYER,0);
    guest(cpu,0x8003A670u,PLAYER,0);guest(cpu,0x8003CD08u,PLAYER,0);
    project(PLAYER+8,inventory+8,8);project(PLAYER+0x18,inventory+0x18,8);
    project(PLAYER+0xA8,inventory+0xA8,32);
    psx_mod_write_byte(PLAYER+0xC9,inventory[0xC9]);
    psx_mod_write_byte(PLAYER+0x93,inventory[0x93]);
    psx_mod_write_byte(PLAYER+0xFB,inventory[0xFB]);
    guest(cpu,0x8003DF20u,PLAYER,inventory[0x93]);guest(cpu,0x8003EE38u,PLAYER,0);
    psx_mod_write_byte(PLAYER+0x5C,psx_mod_read_byte(PLAY+0x47+story_duel.character[seat]));
    psx_mod_write_byte(PLAYER+0x5D,psx_mod_read_byte(PLAYER+0x5C));
    /* Remain hidden like an outgoing passenger until the real NPC intro
     * supplies the incoming destination. scene_tick owns that same beam. */
    psx_mod_write_byte(PLAYER+3,0);psx_mod_write_byte(PLAYER+4,3);
    both_project(was);project(PLAY,play,sizeof play);project(CAMERA,camera,sizeof camera);
    memcpy(saved_play,projection_play,sizeof saved_play);memcpy(saved_camera,projection_camera,sizeof saved_camera);
    memcpy(resources,projection_resources,sizeof resources);
    /* A P2-owned world deliberately keeps the canonical stage palette at
     *SCRATCH+28. A nested body projection must restore that caller override. */
    for(unsigned i=0;i<6;++i)psx_mod_write_word(SCRATCH+resource_offsets[i],caller_resources[i]);
    busy=old_busy;life.status[seat]=MMX5_COOP_ALIVE;life.wipe=0;
    psx_mod_counter_add(seat?"mmx5.coop.duel-p2-revived":"mmx5.coop.duel-p1-revived",1);
}
static void duel_begin(CPUState *cpu,uint32_t actor,unsigned seat) {
    if(seat>1 || life.status[seat]!=MMX5_COOP_ALIVE || !duel_inner_door(actor))return;
    unsigned a=both_body_byte(0,2),b=both_body_byte(1,2);
    if(a>1 || b>1 || a==b)return;
    memset(&story_duel,0,sizeof story_duel);
    story_duel.phase=1;story_duel.door=actor;story_duel.entrant=(uint8_t)seat;
    story_duel.character[0]=(uint8_t)a;story_duel.character[1]=(uint8_t)b;
    duel_revive(cpu,seat^1u);
    /* PvP owns current health only. Earned native maxima/card data stay intact. */
    for(unsigned i=0;i<2;++i) {
        uint8_t *body=i?second.body:first.body;
        body[0x5C]=body[0x5D]=story_duel.health[i]=32;
        if(i==projected) {
            psx_mod_write_byte(PLAYER+0x5C,32);psx_mod_write_byte(PLAYER+0x5D,32);
        }
    }
    life.select_frames=life.select_armed=0;
    psx_mod_counter_add(seat?"mmx5.coop.duel-p2-entry":"mmx5.coop.duel-p1-entry",1);
}
static int duel_result_ready(void) {
    if(!story_duel.winner || story_duel.winner>2)return 0;
    unsigned loser=(story_duel.winner-1u)^1u;
    return life.status[loser]==MMX5_COOP_FALLEN ||
        (life.status[loser]==MMX5_COOP_DYING && both_body_byte(loser,4)==1 &&
         both_body_byte(loser,5)==0x48 && !both_body_byte(loser,0x5C));
}
static int duel_loser_body(void) {
    return duel_scope() && story_duel.phase==4 && story_duel.winner &&
        projected==((story_duel.winner-1u)^1u);
}
static void duel_defeat_begin(CPUState *cpu,unsigned loser) {
    clear_buster_effects(loser);clear_personal_drawables(loser);
    /* Original BIN28 EECC0 initializes stand48 with player animation51:
     * the authored crouched injury pose, for either hero's normal art.
     * Do not enter native38C08/major2, which emits ordinary death orbs. */
    psx_mod_write_byte(PLAYER+0x5C,0);psx_mod_write_byte(PLAYER+0x5D,0);
    psx_mod_write_byte(PLAYER+3,1);psx_mod_write_byte(PLAYER+4,1);
    psx_mod_write_byte(PLAYER+5,0x48);psx_mod_write_byte(PLAYER+6,1);
    psx_mod_write_byte(PLAYER+0xD0,1);psx_mod_write_byte(PLAYER+0xD1,0x48);
    psx_mod_write_byte(PLAYER+0xD3,1);psx_mod_write_byte(PLAYER+0x7A,1);
    psx_mod_write_byte(PLAYER+0xCC,0);psx_mod_write_byte(PLAYER+0xCD,0);
    psx_mod_write_word(PLAYER+0x20,0);psx_mod_write_word(PLAYER+0x24,0);
    psx_mod_write_word(PLAYER+0x28,0);psx_mod_write_word(PLAYER+0x2C,0x4200);
    guest(cpu,0x8003B33Cu,PLAYER,0x51);
    mmx5_coop_fatal(&life,loser);
    psx_mod_counter_add(loser?"mmx5.coop.duel-p2-defeat-crouch":
                              "mmx5.coop.duel-p1-defeat-crouch",1);
}
static void duel_defeat_tick(CPUState *cpu) {
    unsigned loser=(story_duel.winner-1u)^1u;
    if(life.status[loser]!=MMX5_COOP_DYING)return;
    unsigned was=projected;both_project(loser);
    guest(cpu,0x80017248u,PLAYER,0);
    guest(cpu,0x8002DED4u,PLAYER,0);
    /* This encounter's floor is the same flat, native anchor used for its
     * incoming player. The NPC integrator alone does not run PLAYER terrain
     * admission: settle an airborne loser, then keep its feet on that floor. */
    uint32_t floor=foot_aligned_y(story_duel.y,story_duel.box,
                                 psx_mod_read_word(PLAYER+0x68));
    if((int32_t)psx_mod_read_word(PLAYER+12)>=(int32_t)floor) {
        psx_mod_write_word(PLAYER+12,floor);psx_mod_write_word(PLAYER+0x1C,floor);
        psx_mod_write_word(PLAYER+0x24,0);
    }
    uint32_t emitter=psx_mod_read_word(story_duel.opponent+0x80);
    /* Native575D0 ramps the blast toward white and signals PLAY+2A above
     * 16/31 brightness. Both NPC scripts consume that signal into phase3.
     * Hide the loser at that authored flash, while explosions still run,
     * rather than keeping the crouch until the controller finally retires. */
    if(psx_mod_read_byte(story_duel.opponent+5)>=3 &&
       emitter>=0x8009E7A0u && emitter<0x8009F9A0u &&
       !((emitter-0x8009E7A0u)%0x30u) &&
       (!psx_mod_read_byte(emitter) ||
        (psx_mod_read_byte(emitter+1)==0x1A && psx_mod_read_byte(emitter+4)>=4))) {
        psx_mod_write_byte(PLAYER+3,0);psx_mod_write_byte(PLAYER+4,3);
        psx_mod_write_byte(PLAYER+5,0);psx_mod_write_byte(PLAYER+6,0);
        life.status[loser]=MMX5_COOP_FALLEN;
        psx_mod_counter_add("mmx5.coop.duel-defeat-vanished",1);
    }
    both_project(was);
}
static int duel_authored_draw(uint32_t actor) {
    if(!duel_scope() || !story_duel.opponent)return 0;
    if(actor==story_duel.opponent)return 1;
    /* Native intro children draw independently of their retained NPC shell. */
    if(actor>=0x80096D98u && actor<0x80098118u && !((actor-0x80096D98u)%0x9Cu))
        return psx_mod_read_byte(actor+1)==0x24 &&
            (psx_mod_read_byte(actor+2)==0 || psx_mod_read_byte(actor+2)==8) &&
            psx_mod_read_word(actor+0x7C)==story_duel.opponent;
    if(actor>=0x80094398u && actor<0x80096D98u && !((actor-0x80094398u)%0x70u)) {
        unsigned type=psx_mod_read_byte(actor+1),subtype=psx_mod_read_byte(actor+2);
        return psx_mod_read_word(actor+0x50)==story_duel.opponent &&
            ((type==0x14 && subtype<=2) || (type==0x13 && subtype<=2) ||
             (type==0x0B && (subtype==3 || subtype==4)));
    }
    return 0;
}
static void duel_retire_intro_children(CPUState *cpu,uint32_t actor,unsigned kind) {
    for(unsigned i=0;i<96;++i) {
        uint32_t child=0x80094398u+i*0x70u;
        if(!psx_mod_read_byte(child) || psx_mod_read_word(child+0x50)!=actor)continue;
        unsigned type=psx_mod_read_byte(child+1),subtype=psx_mod_read_byte(child+2);
        if((kind==0x40 && type==0x13 && subtype<=2) ||
           (kind==0x41 && type==0x14 && subtype<=2)) {
            guest(cpu,0x8002D6FCu,child,0);
            psx_mod_counter_add("mmx5.coop.duel-npc-charge-retired",1);
        } else if(kind==0x40 && type==0x0B && (subtype==3 || subtype==4)) {
            guest(cpu,0x800F701Cu,child,0);
            psx_mod_counter_add("mmx5.coop.duel-npc-aura-retired",1);
        }
    }
    if(kind==0x41)for(unsigned i=0;i<32;++i) {
        uint32_t child=0x80096D98u+i*0x9Cu;
        if(psx_mod_read_byte(child) && psx_mod_read_byte(child+1)==0x24 &&
           (psx_mod_read_byte(child+2)==0 || psx_mod_read_byte(child+2)==8) &&
           psx_mod_read_word(child+0x7C)==actor) {
            guest(cpu,0x8002D6FCu,child,0);
            psx_mod_counter_add("mmx5.coop.duel-npc-aura-retired",1);
        }
    }
}
static int duel_hold_opponent(CPUState *cpu,uint32_t actor) {
    if(!duel_scope() || actor<0x80092090u || actor>=0x80093DD0u ||
       (actor-0x80092090u)%0x9Cu || !psx_mod_read_byte(actor))return 0;
    unsigned kind=story_duel.character[story_duel.entrant]?0x40u:0x41u;
    if(psx_mod_read_byte(actor+1)!=kind || psx_mod_read_byte(actor+2)>(kind==0x41u))return 0;
    if(story_duel.opponent && story_duel.opponent!=actor)return 0;
    /* Bind the shell before its first native draw, without holding its intro. */
    story_duel.opponent=actor;
    if(story_duel.intro_complete)return story_duel.phase<4 || !duel_result_ready();
    unsigned entrant=story_duel.entrant;
    uint32_t box=psx_mod_read_word(actor+0x68);
    int grounded=(psx_mod_read_byte(actor+0x70)&8)!=0;
    /* Zero's authored intro hovers through the whole conversation. Its
     * standing box is installed before that conversation, but its grounded
     * flag appears only afterward. The arena has the entrant's same flat
     * floor: align that actual grounded foot to the NPC box for the one
     * real player arrival, leaving the hidden NPC's script/physics alone. */
    int hovering=kind==0x41 && psx_mod_read_byte(actor+5)==2 &&
        psx_mod_read_byte(actor+6)>=5 && psx_mod_read_byte(actor+6)<=7 &&
        both_living(entrant) && (both_body_byte(entrant,0x70)&8) &&
        both_body_word(entrant,0x68);
    if(story_duel.phase==1 && psx_mod_read_byte(actor+4)==1 &&
       box>=0x80010000u && box<=0x801FFFFCu && (grounded || hovering)) {
        size_t offset;
        if(!mmx5_coop_duel_palette_offset(psx_mod_read_half(actor+0x42),2,0x4000,&offset))return 0;
        story_duel.x=psx_mod_read_word(actor+8);
        story_duel.y=grounded?psx_mod_read_word(actor+12):
            foot_aligned_y(both_body_word(entrant,12),both_body_word(entrant,0x68),box);
        story_duel.box=box;
        story_duel.damage_table=psx_mod_read_word(actor+0x58);
        uint32_t palette=projected?resources[3]:psx_mod_read_word(SCRATCH+0x28);
        capture(palette+(uint32_t)offset,story_duel.colors,sizeof story_duel.colors);
        story_duel.facing[0]=psx_mod_read_byte(actor+0x14);
        story_duel.facing[1]=psx_mod_read_byte(actor+0x15);
        capture(actor,story_duel.native_body,sizeof story_duel.native_body);
        story_duel.phase=2;
        psx_mod_counter_add("mmx5.coop.duel-grounded-arrival-anchors",1);
    }
    if(story_duel.phase!=2 || psx_mod_read_byte(actor+4)!=1 ||
       !psx_mod_read_byte(actor+0x5C) ||
       psx_mod_read_byte(actor+0x5C)!=psx_mod_read_byte(PLAY+0xA2))return 0;
    if(kind==0x40) {if(!psx_mod_read_byte(actor+0x87))return 0;}
    else {
        unsigned seat=story_duel.entrant;
        if(psx_mod_read_byte(PLAY+0x10) || psx_mod_read_byte(PLAY+0x1C) ||
           both_body_byte(seat,0xD0) || both_body_byte(seat,0xD4) ||
           both_body_byte(seat,0x7A))return 0;
    }
    /* Native introduction really completed. Only now replace its HP bank;
     * the real body may already have arrived during the dialogue. */
    story_duel.intro_complete=1;
    duel_retire_intro_children(cpu,actor,kind);
    psx_mod_write_byte(actor+0x9A,0);
    psx_mod_write_byte(actor+0x5D,(uint8_t)(both_body_byte(story_duel.entrant^1u,0x5C)&127u));
    psx_mod_write_byte(actor+0x5F,0);
    psx_mod_counter_add("mmx5.coop.duel-native-intro-complete",1);
    return 1;
}
static int duel_opponent_tick(CPUState *cpu,uint32_t address) {
    if(duel_scope() && story_duel.phase==4 && cpu->gpr[4]==story_duel.opponent) {
        /* The losing hero's duel crouch replaces ordinary death admission. */
        if(!duel_result_ready()) {cpu->gpr[2]=0;return 1;}
        if(duel_opponent_call)return 0;
        duel_opponent_call=1;
        unsigned before=psx_mod_read_byte(story_duel.opponent+5);
        uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        /* Both original defeat scripts allocate their class1A explosion
         * controller at phase1->2 and retain it at shell+80. The shell is
         * only a story sequencer: its player may have WON. Move that newly
         * authored controller to the actual fallen loser before its first
         * update, preserving its timers, children and result handshake. */
        uint32_t emitter=psx_mod_read_word(story_duel.opponent+0x80);
        if(before==1 && psx_mod_read_byte(story_duel.opponent+5)==2 &&
           emitter>=0x8009E7A0u && emitter<0x8009F9A0u &&
           !((emitter-0x8009E7A0u)%0x30u) && psx_mod_read_byte(emitter) &&
           psx_mod_read_byte(emitter+1)==0x1A) {
            unsigned loser=(story_duel.winner-1u)^1u;
            psx_mod_write_word(emitter+8,both_body_word(loser,8));
            psx_mod_write_word(emitter+12,both_body_word(loser,12));
            psx_mod_counter_add("mmx5.coop.duel-loser-defeat-emitter",1);
        }
        /* Native2D9EC may make its own dying shell visible. Keep its real
         * collider/grounding and script children, hiding only this shell. */
        psx_mod_write_byte(story_duel.opponent+3,0);
        duel_opponent_call=0;cpu->gpr[2]=result;return 1;
    }
    /* P2-owned shared scenes intentionally bypass canonical AI projection.
     * Intercept these two original roots too, so the real NPC is held in
     * either world context after its actual introduction has completed. */
    if(!duel_hold_opponent(cpu,cpu->gpr[4]))return 0;
    cpu->gpr[2]=0;return 1;
}
static int duel_return_anchor(Mmx5CoopReturnAnchor *anchor) {
    if(story_duel.phase!=2 || !story_duel.opponent || !story_duel.box)return 0;
    *anchor=(Mmx5CoopReturnAnchor){story_duel.x,story_duel.y,story_duel.box};
    return 1;
}
static int duel_hud_active(void) {return duel_scope();}
static int duel_shot(uint32_t actor) {
    return actor>=SHOTS && actor<SHOTS+sizeof first.shots && !((actor-SHOTS)%0x9Cu);
}
static int duel_incoming(uint8_t *incoming,const uint8_t *shot,unsigned amount) {
    return mmx5_coop_duel_damage_incoming(incoming,shot,amount,
        (int16_t)psx_mod_read_half(CAMERA+10),(int16_t)psx_mod_read_half(CAMERA+14));
}
/* Native31F60 is reached only after31DD0's real attack overlap. Verify the
 * corresponding native PLAYER overlap before its token/weakness writes. */
static int duel_private_contact(CPUState *cpu,uint32_t address) {
    if(!duel_contact_call)return balance_private_contact(cpu,address);
    if(duel_native_contact)return 0;
    if(duel_transfer_failed) {cpu->gpr[2]=0;return 1;}
    if(cpu->gpr[4]!=story_duel.opponent || !duel_shot(cpu->gpr[5])) {
        cpu->gpr[2]=0;return 1;
    }
    uint8_t shot[0x9C],incoming[0x9C],proxy[0x9C],body[0x158];
    capture(cpu->gpr[5],shot,sizeof shot);
    uint32_t table=psx_mod_read_word(story_duel.opponent+0x58);
    /* Both original story tables have80 signed nonnegative damage entries.
     * Reject unsupported live status/index data before native token writes. */
    unsigned amount=mmx5_coop_duel_attack_damage(shot[1]);
    if(!amount || shot[1]>=0x50 ||
       !duel_incoming(incoming,shot,1)) {cpu->gpr[2]=0;return 1;}
    capture(story_duel.opponent,proxy,sizeof proxy);
    both_project(duel_attacker^1u);capture(PLAYER,body,sizeof body);
    unsigned allowed=mmx5_coop_duel_damage_ready(body) && !psx_mod_read_byte(0x800C9318u);
    if(allowed) {
        project(story_duel.opponent,incoming,sizeof incoming);
        allowed=guest(cpu,0x8002E274u,story_duel.opponent,PLAYER)!=0;
    }
    project(story_duel.opponent,proxy,sizeof proxy);both_project(duel_attacker);
    if(!allowed) {cpu->gpr[2]=0;return 1;}
    uint32_t shot_address=cpu->gpr[5];uint8_t attack_token=psx_mod_read_byte(PLAYER+0xCD);
    /* Story weakness/reaction rows reject valid player moves before32184.
     * Use an ordinary positive row only for this proven contact transaction,
     * then restore it immediately; no native table is persistently changed. */
    uint32_t row=table+shot[1]*2u;uint16_t original=psx_mod_read_half(row);
    psx_mod_write_half(row,(uint16_t)(amount<<8));
    duel_native_contact=1;
    uint32_t result=guest(cpu,address,story_duel.opponent,shot_address);
    duel_native_contact=0;
    psx_mod_write_half(row,original);
    if(duel_transfer_failed) {
        project(shot_address,shot,sizeof shot);project(story_duel.opponent,proxy,sizeof proxy);
        psx_mod_write_byte(PLAYER+0xCD,attack_token);result=0;
    }
    cpu->gpr[2]=result;return 1;
}
static int duel_projectile_result(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!duel_contact_call || !duel_transfer_failed)return 0;
    /* The enclosing31F60 transaction restores its real shot/target/tokens.
     * Do not let a rejected injury consume a projectile or emit hit SFX. */
    cpu->gpr[2]=0;return 1;
}
/* This instruction precedes native projectile consumption. Use real hero
 * defense/injury now, so armor-adjusted lethality reaches native321EC. */
static void duel_damage(CPUState *cpu,uint32_t address) {
    if(!duel_contact_call) {balance_damage(cpu,address);return;}
    uint32_t actor=story_duel.opponent,shot_address=cpu->gpr[5];
    uint8_t shot[0x9C],incoming[0x9C],proxy[0x9C],before[0x158],after[0x158];
    if(cpu->gpr[7]!=actor || !duel_shot(shot_address))return;
    /* Native zero table damage is an accepted harmless contact. Retain its
     * token/disposition without inventing an injury or disabling co-op. */
    capture(shot_address,shot,sizeof shot);
    unsigned amount=mmx5_coop_duel_attack_damage(shot[1]);
    if(!duel_incoming(incoming,shot,amount)) {
        duel_transfer_failed=1;cpu->gpr[2]=psx_mod_read_byte(actor+0x5C);return;
    }
    capture(actor,proxy,sizeof proxy);
    both_project(duel_attacker^1u);capture(PLAYER,before,sizeof before);
    uint8_t lock=psx_mod_read_byte(PLAY+0x1C);
    project(actor,incoming,sizeof incoming);
    duel_native_injury=1;
    uint32_t result=guest(cpu,0x800317B8u,actor,0);
    duel_native_injury=0;
    capture(PLAYER,after,sizeof after);unsigned remaining=0;
    if(!mmx5_coop_duel_damage_result(before,after,result,&remaining)) {
        /* Positive native317B8 changes only this body and scoped rank/lock
         * state. Roll back before the enclosing contact rejects consumption. */
        project(PLAYER,before,sizeof before);
        duel_transfer_failed=1;
        remaining=before[0x5C]&127u;
    } else {
        story_duel.health[duel_attacker^1u]=(uint8_t)remaining;
        psx_mod_counter_add(duel_attacker?"mmx5.coop.duel-p2-injuries":
                                        "mmx5.coop.duel-p1-injuries",1);
        if(!remaining && story_duel.phase==3) {
            /* The first genuinely accepted fatal in canonical contact
             * order resolves this encounter once, before any second scan.
             * This also gives simultaneous attacks a deterministic result. */
            story_duel.winner=(uint8_t)(duel_attacker+1);
            duel_story_character=(uint8_t)(story_duel.character[duel_attacker]+1);
            story_duel.phase=4;
            duel_defeat_begin(cpu,duel_attacker^1u);
            psx_mod_counter_add(duel_attacker?"mmx5.coop.duel-p2-wins":
                                            "mmx5.coop.duel-p1-wins",1);
        }
    }
    /* A real duel fatal is resolved by encounter ownership, not team retry. */
    psx_mod_write_byte(PLAY+0x1C,lock);
    project(actor,proxy,sizeof proxy);both_project(duel_attacker);
    cpu->gpr[2]=remaining;
}
static int duel_injury_amount(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!duel_native_injury || !duel_contact_call || !duel_scope())return 0;
    /* Native31670 scales for difficulty/armor/Black Zero/parts. Preserve
     * those earned states and normal reactions, applying the PvP amount once. */
    cpu->gpr[2]=cpu->gpr[5];return 1;
}
static void duel_contacts(CPUState *cpu) {
    if(story_duel.phase!=3 || scene_owner || psx_mod_read_byte(PLAY+1) ||
       psx_mod_read_byte(PLAY+0x1C) || psx_mod_read_byte(PLAY+0x0F) ||
       contact_call || projected || world_owner || failed)return;
    uint32_t actor=story_duel.opponent;
    /* These are the original normal story tables: BIN28 F2668..F2680
     * installs X764C0; Zero installs76560. Require the retained actual NPC
     * to corroborate its character's table before enabling either direction. */
    static const uint32_t tables[2]={0x800764C0u,0x80076560u};
    unsigned boss_character=story_duel.character[story_duel.entrant^1u];
    if(story_duel.damage_table!=tables[boss_character])return;
    uint8_t npc[0x9C],proxy[0x9C],camera[0xFC];capture(actor,npc,sizeof npc);
    capture(CAMERA,camera,sizeof camera);
    uint32_t hits=psx_mod_read_word(PLAY+0xB4),hurt=psx_mod_read_word(PLAY+0xB8);
    uint32_t maximum=psx_mod_read_word(PLAY+0xBC);
    capture_player(&first);contact_call=duel_contact_call=1;duel_transfer_failed=0;
    for(unsigned attacker=0;attacker<2 && story_duel.phase==3 && !duel_transfer_failed;++attacker) {
        unsigned defender=attacker^1u;
        if(life.status[attacker]!=MMX5_COOP_ALIVE || life.status[defender]!=MMX5_COOP_ALIVE)continue;
        both_project(attacker);
        if(split_views && split_valid)project(CAMERA,split_cameras[attacker],sizeof camera);
        const uint8_t *body=defender?second.body:first.body;
        if(!mmx5_coop_duel_damage_target(proxy,body,attacker,defender,
              tables[story_duel.character[defender]],story_duel.token[defender]))continue;
        project(actor,proxy,sizeof proxy);duel_attacker=attacker;
        guest(cpu,0x80031DD0u,actor,0);
        story_duel.token[defender]=psx_mod_read_byte(actor+0x65);
    }
    both_project(0);project(actor,npc,sizeof npc);project(CAMERA,camera,sizeof camera);
    if(story_duel.phase==4) {
        /* Admit the genuine result through the original NPC root's defeat
         * state, exactly as F291C/F2924 or F7864..F7878 would. Native code
         * owns grounding, timers, UI children, text completion and request10.
         * The existing allied survivor world projects the actual winner. */
        psx_mod_write_byte(actor+0x5C,0x80);
        psx_mod_write_byte(actor+4,2);psx_mod_write_byte(actor+5,0);
        psx_mod_write_byte(actor+6,0);
    }
    /* A PvP hit is not a campaign enemy hit, rank bonus or damage penalty. */
    psx_mod_write_word(PLAY+0xB4,hits);psx_mod_write_word(PLAY+0xB8,hurt);
    psx_mod_write_word(PLAY+0xBC,maximum);
    contact_call=duel_contact_call=duel_attacker=0;
    if(duel_transfer_failed) {
        failed=1;psx_mod_counter_add("mmx5.coop.duel-injury-invariant-failed",1);
    }
}
static void duel_after_world(CPUState *cpu) {
    if(!duel_scope() || !story_duel.intro_complete || !story_duel.opponent ||
       psx_mod_local_view_scope())return;
    /* Regeneration and any unforeseen heal cannot increase PvP health. */
    for(unsigned i=0;i<2;++i) {
        uint8_t *body=i?second.body:first.body;
        unsigned raw=both_body_byte(i,0x5C),hp=raw&127u;
        if(hp>story_duel.health[i]) {
            body[0x5C]=(uint8_t)((raw&128u)|story_duel.health[i]);
            if(i==projected)psx_mod_write_byte(PLAYER+0x5C,body[0x5C]);
        } else story_duel.health[i]=(uint8_t)hp;
    }
    duel_contacts(cpu);
    if(story_duel.phase==4) {duel_defeat_tick(cpu);return;}
    unsigned seat=story_duel.entrant^1u;
    /* Native fill has completed. From here the ordinary player's body owns
     * HP; the hidden NPC is only the native right-hand meter's reference. */
    unsigned hp=both_body_byte(seat,0x5C);
    /* Native pending damage marks surviving HP with80 too; only80 itself
     * is fatal. A nonlethal injury must never briefly empty the boss meter. */
    psx_mod_write_byte(story_duel.opponent+0x5C,(uint8_t)(hp&0x7Fu));
    /* Native fill can leave its red damage trail at48/96. That authored
     * health is not a PvP injury: keep the trail within the32HP contract. */
    if(psx_mod_read_byte(story_duel.opponent+0x5D)>32)
        psx_mod_write_byte(story_duel.opponent+0x5D,32);
    psx_mod_write_byte(PLAY+0xA2,32);
}
/* Project display-only maxima during the original HUD call. The native
 * intro keeps its actual fill target/readiness; campaign maxima never change. */
static uint32_t duel_native_hud(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1) {
    if(!duel_scope())return guest(cpu,address,a0,a1);
    uint8_t x=psx_mod_read_byte(PLAY+0x47),zero=psx_mod_read_byte(PLAY+0x48);
    uint8_t maximum=psx_mod_read_byte(PLAY+0xA2);
    uint8_t player_hp[4],boss_hp[4];
    int resolved=story_duel.phase==4 && story_duel.winner;
    if(resolved) {
        /* Native story completion always defeats the hidden NPC shell.
         * Display the real participants' result instead: only the loser
         * empties, independent of physical seat, entrant or survivor world. */
        capture(PLAYER+0x5C,player_hp,sizeof player_hp);
        capture(story_duel.opponent+0x5C,boss_hp,sizeof boss_hp);
        unsigned entrant_hp=story_duel.health[story_duel.entrant];
        unsigned opponent_hp=story_duel.health[story_duel.entrant^1u];
        psx_mod_write_byte(PLAYER+0x5C,(uint8_t)entrant_hp);
        psx_mod_write_byte(PLAYER+0x5D,(uint8_t)entrant_hp);
        psx_mod_write_byte(story_duel.opponent+0x5C,(uint8_t)opponent_hp);
        psx_mod_write_byte(story_duel.opponent+0x5D,(uint8_t)opponent_hp);
    }
    psx_mod_write_byte(PLAY+0x47,32);psx_mod_write_byte(PLAY+0x48,32);psx_mod_write_byte(PLAY+0xA2,32);
    uint32_t begin=psx_mod_read_word(SCRATCH+0x110),result=guest(cpu,address,a0,a1);
    uint32_t end=psx_mod_read_word(SCRATCH+0x110);
    if(end>=begin && end-begin<=0x1000 && !((end-begin)%36))
        for(uint32_t at=begin;at<end;at+=36)
            if(psx_mod_read_half(at+8)==288 && psx_mod_read_half(at+16)==291 &&
               psx_mod_read_half(at+24)==288 && psx_mod_read_half(at+32)==291)
                for(unsigned xy=8;xy<=32;xy+=8)
                    psx_mod_write_half(at+xy+2,(uint16_t)(psx_mod_read_half(at+xy+2)+29));
    psx_mod_write_byte(PLAY+0x47,x);psx_mod_write_byte(PLAY+0x48,zero);psx_mod_write_byte(PLAY+0xA2,maximum);
    if(resolved) {
        project(PLAYER+0x5C,player_hp,sizeof player_hp);
        project(story_duel.opponent+0x5C,boss_hp,sizeof boss_hp);
    }
    return result;
}
static int duel_personal_hud(CPUState *cpu,uint32_t address) {
    if(!hud_call || !duel_hud_active() || hud_seat!=(story_duel.entrant^1u))return 0;
    if(address==0x800262D8u && cpu->gpr[5])return 0; /* Keep the right boss bar. */
    cpu->gpr[2]=0;return 1;
}

static int duel_text(CPUState *cpu,uint32_t address) {
    if(intro_boss_text_skip(cpu,address))return 1;
    unsigned id=cpu->gpr[4]&0x7FFFu;
    if(duel_text_call || story_duel.phase!=4 || !story_duel.winner ||
       !duel_story_character || duel_story_character>2 ||
       !duel_scope() || (id!=0x43 && id!=0x45 && id!=0x47))
        return capsule_text(cpu,address);
    /* These three IDs belong to this native post-duel result, including
     * queued request replay. Keep the native textbox/cursor lifetime. */
    unsigned character=duel_story_character-1u;
    uint32_t requested=(cpu->gpr[4]&~0x7FFFu) |
        (character?0x43u:(psx_mod_read_byte(PLAY+0xCB)&0x80u)?0x45u:0x47u);
    uint8_t campaign=psx_mod_read_byte(PLAY+0x44);
    uint32_t messages=psx_mod_read_word(SCRATCH+0x48);
    duel_text_call=1;
    psx_mod_write_byte(PLAY+0x44,(uint8_t)character);
    psx_mod_write_word(SCRATCH+0x48,assets[character].message_root);
    uint32_t result=guest(cpu,address,requested,cpu->gpr[5]);
    psx_mod_write_byte(PLAY+0x44,campaign);
    psx_mod_write_word(SCRATCH+0x48,messages);
    duel_text_call=0;cpu->gpr[2]=result;
    psx_mod_counter_add(character?"mmx5.coop.duel-zero-story-text":
                                "mmx5.coop.duel-x-story-text",1);
    return 1;
}
static void duel_story_branch(CPUState *cpu,uint32_t address) {
    (void)address;
    /* Original205AC reaches this loaded character predicate only after
     * the final raw12 victory request. Leave player/body/parts/card fields
     * alone; new-game/native card-load frontend clears the separate latch. */
    if(duel_story_character && psx_mod_read_byte(PLAY+0x0C)==12 &&
       psx_mod_read_byte(PLAY+0x0F) && !psx_mod_local_view_scope())
        cpu->gpr[2]=duel_story_character-1u;
}
