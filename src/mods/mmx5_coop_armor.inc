/* Reuse the original six-slot selector. P1 chooses either hero; P2 chooses
 * the remaining hero. State2 lets P1 choose Normal/Black after choosing Zero,
 * without overwriting native control bytes immediately after the six slots. */
int mmx5_coop_zero_form(void) {
    /* Availability belongs to the genuine capsule/code progress byte, which
     * the original memory-card serializer already preserves. A preference
     * or a stale form from another campaign must never unlock Black Zero. */
    if(!(psx_mod_read_byte(PLAY+0x4A)&0x10u))return 0;
    if(armor_roster_active && psx_mod_read_word(0x800EE970u)==24u &&
       psx_mod_read_byte(PLAY)==4 && psx_mod_read_byte(PLAY+1)==4 &&
       (armor_selecting_seat==2 ||
        (armor_selecting_seat==1 && !psx_mod_read_byte(PLAY+0x44))))
        return psx_mod_read_byte(PLAY+0x30)==1;
    return selected_zero_form!=0;
}
static void armor_filter_list(void) {
    unsigned count=psx_mod_read_byte(0x800F6DFBu);
    if(!count || count>6)return;
    uint8_t list[6];capture(0x800F6DF0u,list,sizeof list);
    unsigned cursor=psx_mod_read_byte(PLAY+0x30);
    int changed=armor_list_seat!=armor_selecting_seat+1;
    unsigned selected=cursor<count?list[cursor]:selected_x_armor;
    if(!armor_roster_active) {
        memcpy(armor_native_roster,list,sizeof list);armor_native_count=count;
        armor_roster_active=1;changed=1;
    }
    unsigned character=armor_selecting_seat==2?1:
        armor_selecting_seat==1?(psx_mod_read_byte(PLAY+0x44)^1u):2u,next=0;
    if(character==1) {
        /* Both use genuine roster5. Native Black Zero reads select its
         * authored preview palette and name animation7, without roster6. */
        memset(list,0,sizeof list);list[0]=5;
        count=(psx_mod_read_byte(PLAY+0x4A)&0x10u)?2u:1u;
        if(count==2)list[1]=5;
        next=changed?selected_zero_form:cursor<count?cursor:0;
        if(next>=count)next=0;
    } else {
        if(changed || (selected>4 && character!=2))selected=selected_x_armor;
        memcpy(list,armor_native_roster,sizeof list);
        count=mmx5_coop_armor_list(list,armor_native_count);
        if(character==2 && mmx5_coop_zero_available(psx_mod_read_byte(PLAY+0x79),
                psx_mod_read_byte(PLAY+0xCB),keep_zero))list[count++]=5;
        if(character==2 && changed && psx_mod_read_byte(PLAY+0x44))selected=5;
        for(unsigned i=0;i<count;++i)if(list[i]==selected) {next=i;break;}
    }
    armor_list_seat=armor_selecting_seat+1;
    project(0x800F6DF0u,list,sizeof list);
    psx_mod_write_byte(0x800F6DFBu,(uint8_t)count);
    psx_mod_write_byte(PLAY+0x30,(uint8_t)next);
    psx_mod_write_byte(PLAY+0x2A,list[next]);
}
static void armor_restore_list(void) {
    if(!armor_roster_active)return;
    /* Parts and other original menus need the genuine complete roster. */
    project(0x800F6DF0u,armor_native_roster,sizeof armor_native_roster);
    psx_mod_write_byte(0x800F6DFBu,(uint8_t)armor_native_count);
    armor_roster_active=armor_list_seat=armor_selecting_seat=0;
}
static void armor_refresh_zero_preview(void) {
    /* Two native class2B previews cache roster5 in54. Normal/Black share
     * that roster: invalidate only their cached appearance on a change.
     * Their original updates select art/name/color with no extra clock. */
    for(uint32_t actor=0x800D1F40u;actor<0x800D4F40u;actor+=0x60u)
        if(psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==0x2Bu &&
           psx_mod_read_byte(actor+2)<=1 && psx_mod_read_byte(actor+4))
            psx_mod_write_byte(actor+0x54,255);
}
static int armor_stage_ui(CPUState *cpu,uint32_t address) {
    if(armor_ui_call || failed || psx_mod_local_view_scope() ||
       psx_mod_read_word(0x800EE970u)!=24 || psx_mod_read_byte(PLAY)!=4 ||
       cpu->gpr[4]!=PLAY)return 0;
    unsigned primary=psx_mod_read_byte(PLAY+0x44);
    if(primary>1)return 0;
    if(!armor_selection_seen) {
        unsigned native=psx_mod_read_byte(PLAY+0x49);
        if(!primary && native<5)selected_x_armor=native;
        armor_selection_seen=1;
    }
    unsigned phase=psx_mod_read_byte(PLAY+2);
    unsigned choosing=psx_mod_read_byte(PLAY+1)==4 && phase==5;
    uint16_t edges[2];
    for(unsigned seat=0;seat<2;++seat) {
        uint16_t held=input(seat);edges[seat]=(uint16_t)(held&~armor_previous_input[seat]);
        armor_previous_input[seat]=held;
    }
    uint16_t native_edges=psx_mod_read_half(PAD+4);
    if(psx_mod_read_byte(PLAY+1)==4)armor_filter_list();else armor_restore_list();
    unsigned zero_before=(unsigned)mmx5_coop_zero_form();
    if(choosing) {
        unsigned seat=armor_selecting_seat==1;
        psx_mod_write_half(PAD+4,mmx5_coop_armor_input(edges[seat],edges[seat]));
        psx_mod_counter_add(seat?"mmx5.coop.armor-p2-input-frames":"mmx5.coop.armor-p1-input-frames",1);
    }
    armor_ui_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    armor_ui_call=0;
    if(choosing) {
        psx_mod_write_half(PAD+4,native_edges);
        unsigned next=psx_mod_read_byte(PLAY+2);
        if(next==6) {
            unsigned choice=psx_mod_read_byte(PLAY+0x49);
            if(!armor_selecting_seat) {
                /* Keep the original confirmation's chosen P1 hero. The
                 * partner selector must not replace that campaign identity. */
                primary=choice==5;
                psx_mod_write_byte(PLAY+0x44,(uint8_t)primary);
                if(!primary)selected_x_armor=choice;
                else if(!(psx_mod_read_byte(PLAY+0x4A)&0x10u))selected_zero_form=0;
                armor_selecting_seat=primary && (psx_mod_read_byte(PLAY+0x4A)&0x10u)?2:1;
                psx_mod_write_byte(PLAY+2,4);
            } else {
                if(armor_selecting_seat==2 || !primary)
                    selected_zero_form=psx_mod_read_byte(PLAY+0x30)==1;
                else if(choice<5)selected_x_armor=choice;
                psx_mod_write_byte(PLAY+0x44,(uint8_t)primary);
                psx_mod_write_byte(PLAY+0x49,(uint8_t)(primary?5:selected_x_armor));
                if(armor_selecting_seat==2) {
                    armor_selecting_seat=1;psx_mod_write_byte(PLAY+2,4);
                }
            }
            psx_mod_counter_add("mmx5.coop.armor-confirmed",1);
        } else if(next==8 && armor_selecting_seat) {
            armor_selecting_seat=0;psx_mod_write_byte(PLAY+2,4);
        }
    }
    if(psx_mod_read_word(0x800EE970u)==24) {
        /* Original F0ED8 initializes the first native list index. Once its
         * preview actors exist, restore this seat's last equipped form. */
        if(psx_mod_read_byte(PLAY+1)==4 && phase==3)armor_list_seat=0;
        if(psx_mod_read_byte(PLAY+1)==4)armor_filter_list();else armor_restore_list();
        if(zero_before!=(unsigned)mmx5_coop_zero_form())armor_refresh_zero_preview();
    }
    cpu->gpr[2]=result;return 1;
}
