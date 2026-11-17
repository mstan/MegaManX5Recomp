/* Reuse the original selector. P1 chooses either hero; P2 chooses the
 * remaining hero. Both Zero entries use genuine roster5, with their list
 * positions selecting the native normal/Black preview and appearance. */
static int armor_black_available(void) {
    /* Session code mods expose a choice without fabricating a capsule grant
     * or storing cheat progress in the native memory card. */
    return (psx_mod_read_byte(PLAY+0x4A)&0x10u) || mmx5_coop_forms_requested(1);
}
int mmx5_coop_armor_selection_owned(void) {return armor_selection_seen!=0;}
static unsigned armor_x_list(uint8_t *list,unsigned count) {
    count=mmx5_coop_armor_list(list,count);
    if(mmx5_coop_forms_requested(0)) {
        for(unsigned i=0;i<count;++i)if(list[i]==4)return count;
        list[count++]=4;
    }
    return count;
}
int mmx5_coop_zero_form(void) {
    if(!armor_black_available())return 0;
    if(armor_roster_active && psx_mod_read_word(0x800EE970u)==24u &&
       psx_mod_read_byte(PLAY)==4 &&
       psx_mod_read_byte(PLAY+0x2A)==5) {
        unsigned menu=psx_mod_read_byte(PLAY+1);
        if(menu!=4 && menu!=5 && menu!=6)return selected_zero_form!=0;
        unsigned cursor=psx_mod_read_byte(menu==4?PLAY+0x30:0x800F6DF6u);
        return cursor && cursor<7 && psx_mod_read_byte(0x800F6DF0u+cursor-1)==5;
    }
    return selected_zero_form!=0;
}
static void armor_filter_parts_list(void) {
    if(!armor_roster_active)return;
    armor_list_seat=0;
    uint8_t list[7]={0};memcpy(list,armor_native_roster,sizeof armor_native_roster);
    unsigned count=armor_x_list(list,armor_native_count);
    if(mmx5_coop_zero_available(psx_mod_read_byte(PLAY+0x79),
            psx_mod_read_byte(PLAY+0xCB),keep_zero)) {
        list[count++]=5;
        if(armor_black_available())list[count++]=5;
    }
    /* Keep roster5 for the original four-slot capacity, art and part rules.
     * Audited equipment accesses below select each form's persistent row.
     * The seventh choice is virtual because +6 is the native Parts cursor. */
    project(0x800F6DF0u,list,6);
    psx_mod_write_byte(0x800F6DFBu,(uint8_t)count);
    unsigned cursor=psx_mod_read_byte(0x800F6DF6u);
    if(cursor>=count)psx_mod_write_byte(0x800F6DF6u,0);
}
static uint32_t armor_parts_address(const CPUState *cpu,uint32_t pc,
        uint32_t instruction,uint32_t address) {
    (void)cpu;(void)instruction;
    if(failed)return address;
    int native_body=pc==0x8003D608u || pc==0x8003D7BCu;
    if(!native_body && (psx_mod_read_word(0x800EE970u)!=24u ||
                       psx_mod_read_byte(PLAY)!=4))return address;
    /* Gaea has zero native part slots. Its otherwise unused equipment word
     * travels through the existing six-row card serializer and checksum.
     * Reuse that storage for Black Armor, while all actual Gaea consumers
     * continue to see an empty row. No card layout or other progress changes. */
    if(address==PLAY+0x94u)return armor_empty_parts;
    if(address==PLAY+0x9Cu && mmx5_coop_zero_form() &&
       (!native_body || psx_mod_read_byte(PLAYER+2)==1))return PLAY+0x94u;
    return address;
}
static int armor_name_draw(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(armor_name_call || failed || psx_mod_read_byte(PLAY)!=4 ||
       psx_mod_read_word(0x800EE970u)!=24 ||
       psx_mod_read_byte(PLAY+0x2A)!=5 || !mmx5_coop_zero_form() ||
       actor<0x800D1F40u || actor>=0x800D4F40u ||
       (actor-0x800D1F40u)%0x60u || !psx_mod_read_byte(actor))return 0;
    unsigned type=psx_mod_read_byte(actor+1),subtype=psx_mod_read_byte(actor+2);
    int unit=type==0x2B && !subtype && psx_mod_read_byte(PLAY+1)==4;
    int parts=type==0x0F && subtype==1 &&
        (psx_mod_read_byte(PLAY+1)==5 || psx_mod_read_byte(PLAY+1)==6);
    if(!unit && !parts)return 0;
    /* Use the same native italic glyphs as the original armor nameplates.
     * This is an ordinary native sprite group, with the original camera,
     * palette, priority, packet painter and menu transitions. */
    static const char name[]="BLACK ARMOR";
    unsigned count=0;
    for(unsigned i=0;name[i];++i)if(name[i]!=' ') {
        unsigned letter=(unsigned)(name[i]-'A');
        uint16_t tile=(uint16_t)(0x210u+(letter/8u)*16u+letter%8u);
        uint32_t at=armor_name_frame+4u+count++*4u;
        psx_mod_write_byte(at,(uint8_t)(tile>>8));psx_mod_write_byte(at+1,(uint8_t)tile);
        psx_mod_write_byte(at+2,(uint8_t)(40-44+(int)i*8));
        psx_mod_write_byte(at+3,(uint8_t)-8);
    }
    psx_mod_write_half(armor_name_frame,(uint16_t)count);
    psx_mod_write_half(armor_name_frame+2,1);
    uint32_t original=psx_mod_read_word(actor+0x3C);
    uint8_t frame=psx_mod_read_byte(actor+0x47);
    psx_mod_write_word(actor+0x3C,armor_name_frame);psx_mod_write_byte(actor+0x47,0);
    psx_mod_counter_add("mmx5.coop.armor-name-draw",1);
    armor_name_call=1;uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);armor_name_call=0;
    psx_mod_write_word(actor+0x3C,original);psx_mod_write_byte(actor+0x47,frame);
    cpu->gpr[2]=result;return 1;
}
static void armor_filter_list(void) {
    unsigned count=psx_mod_read_byte(0x800F6DFBu);
    if(!count || count>(armor_roster_active?7u:6u))return;
    uint8_t list[7]={0};capture(0x800F6DF0u,list,6);
    /* The next native byte is the Parts cursor, not a seventh list cell.
     * A fully unlocked roster's final Black Zero entry is virtual: normal
     * navigation still advances the original cursor/count, and this same
     * wrapper supplies roster5 before previews or confirmation consume it. */
    if(count==7)list[6]=5;
    unsigned cursor=psx_mod_read_byte(PLAY+0x30);
    int changed=armor_list_seat!=armor_selecting_seat+1;
    unsigned selected=cursor<count?list[cursor]:selected_x_armor;
    unsigned form=selected==5 && cursor && list[cursor-1]==5;
    if(!armor_roster_active) {
        memcpy(armor_native_roster,list,sizeof armor_native_roster);armor_native_count=count;
        armor_roster_active=1;changed=1;
    }
    unsigned character=armor_selecting_seat==2?1:
        armor_selecting_seat==1?(psx_mod_read_byte(PLAY+0x44)^1u):2u,next=0;
    if(character==1) {
        /* Both use genuine roster5. Native Black Zero reads select its
         * authored preview palette and name animation7, without roster6. */
        memset(list,0,sizeof list);list[0]=5;
        count=armor_black_available()?2u:1u;
        if(count==2)list[1]=5;
        next=changed?selected_zero_form:cursor<count?cursor:0;
        if(next>=count)next=0;
    } else {
        if(changed || (selected>4 && character!=2))selected=selected_x_armor;
        memcpy(list,armor_native_roster,sizeof armor_native_roster);list[6]=0;
        count=armor_x_list(list,armor_native_count);
        if(character==2 && mmx5_coop_zero_available(psx_mod_read_byte(PLAY+0x79),
                psx_mod_read_byte(PLAY+0xCB),keep_zero)) {
            list[count++]=5;
            if(armor_black_available())list[count++]=5;
        }
        if(character==2 && changed && psx_mod_read_byte(PLAY+0x44)) {
            selected=5;form=selected_zero_form;
        }
        for(unsigned i=0;i<count;++i)if(list[i]==selected) {next=i;break;}
        if(selected==5 && form && next+1<count && list[next+1]==5)++next;
    }
    armor_list_seat=armor_selecting_seat+1;
    project(0x800F6DF0u,list,6);
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
    host_osd_set_status(NULL);
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
static void armor_virtual_entry(CPUState *cpu,uint32_t address) {
    /* F0024 reads the selected list byte immediately before the original
     * preview/confirmation path. Keep its seventh logical entry out of the
     * adjacent native Parts cursor, without changing the roster ID or UI. */
    if(armor_roster_active && !failed &&
       psx_mod_read_word(0x800EE970u)==24u &&
       psx_mod_read_byte(PLAY)==4 && psx_mod_read_byte(PLAY+1)==4 &&
       address==0x800F0028u && psx_mod_read_word(address)==0x30820840u &&
       psx_mod_read_word(address-4)==0x90630000u &&
       psx_mod_read_byte(PLAY+2)==5 && psx_mod_read_byte(PLAY+0x30)==6 &&
       psx_mod_read_byte(0x800F6DFBu)==7)cpu->gpr[3]=5;
    if(armor_roster_active && !failed && psx_mod_read_byte(PLAY)==4 &&
       (psx_mod_read_byte(PLAY+1)==5 || psx_mod_read_byte(PLAY+1)==6) &&
       psx_mod_read_word(0x800EE970u)==24u &&
       psx_mod_read_byte(0x800F6DFBu)==7 && psx_mod_read_byte(0x800F6DF6u)==6 &&
       ((address==0x800F02E4u && psx_mod_read_word(address)==0x94830004u &&
         psx_mod_read_word(address-4)==0x90650000u) ||
        (address==0x800F1144u && psx_mod_read_word(address)==0x3C03800Du &&
         psx_mod_read_word(address-4)==0x90450000u)))cpu->gpr[5]=5;
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
    unsigned menu=psx_mod_read_byte(PLAY+1);
    if(menu==4)armor_filter_list();
    else if(menu==5 || menu==6)armor_filter_parts_list();
    else armor_restore_list();
    if(menu==5 || menu==6)
        host_osd_set_status(psx_mod_read_byte(PLAY+0x2A)!=5?NULL:
            mmx5_coop_zero_form()?"Black Armor - 4 parts":"Zero - 4 parts");
    unsigned zero_before=(unsigned)mmx5_coop_zero_form();
    if(choosing) {
        unsigned seat=armor_selecting_seat==1;
        if(!armor_selecting_seat)
            host_osd_set_status(psx_mod_read_byte(PLAY+0x2A)!=5?
                "Player 1: Choose character - Left / Right, Confirm":
                !armor_black_available()?
                "Player 1: Zero - Confirm":mmx5_coop_zero_form()?
                "Player 1: Black Armor - Left / Right, Confirm":
                "Player 1: Normal Zero - Left / Right, Confirm");
        else host_osd_set_status(primary?
                "Player 2: Choose X armor - Left / Right, Confirm":
                mmx5_coop_zero_form()?
                "Player 2: Black Armor - Left / Right, Confirm":
                "Player 2: Normal Zero - Left / Right, Confirm");
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
                else selected_zero_form=zero_before;
                armor_selecting_seat=1;
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
        menu=psx_mod_read_byte(PLAY+1);
        if(menu==4)armor_filter_list();
        else if(menu==5 || menu==6)armor_filter_parts_list();
        else armor_restore_list();
        if(zero_before!=(unsigned)mmx5_coop_zero_form())armor_refresh_zero_preview();
    }
    cpu->gpr[2]=result;return 1;
}
