/* Included by the title plugin after guest/context helpers. Original native
 * callbacks still own collision, reaction, timer cleanup and progression. */
#include "mmx5_coop_damage.h"
#define BALANCE_DISPATCH 0x8F7FFF08u
#define BOSS_REJECTION_TABLE 0x80074EE0u
typedef struct {
    uint8_t stage,section,cls,logical,timer_offset,controller,flash_mask,flash_zero;
    uint32_t root,root_word,table,live,install,install_word;
} BossBalanceDescriptor;
static const BossBalanceDescriptor balance_descriptors[9]={
    {1,0,0x36,5,0x80,0,1,1,0x800F9CB4u,0x27BDFFE8u,0x800FDAB8u,0x800F86D8u,0x800F8870u,0xA2020080u},
    {2,1,0x39,7,0x90,0,2,0,0x800F74D8u,0x27BDFFE8u,0x800FA0ECu,0x800F3414u,0x800F351Cu,0xA2220090u},
    {3,0,0x38,8,0x8A,0,1,0,0x800F4F08u,0x27BDFFE8u,0x80101C3Cu,0x800F4CDCu,0x800F4E08u,0xA202008Au},
    {4,1,0x34,10,0x8E,1,2,0,0x800F9550u,0x27BDFFE8u,0x800FF540u,0x800F7DF0u,0x800F7EC4u,0xA462008Eu},
    {5,0,0x32,11,0x88,1,2,0,0x800FB6FCu,0x27BDFFE8u,0x80101C18u,0x800F991Cu,0x800F9A00u,0xA0620088u},
    {6,1,0x35,13,0x94,0,2,0,0x800F53A4u,0x27BDFFE8u,0x800FAD94u,0x800F27A0u,0x800F2828u,0xA2020094u},
    {7,0,0x33,14,0x84,0,1,1,0x800F4B08u,0x27BDFFE8u,0x800FBE20u,0x800F320Cu,0x800F33A4u,0xA2020084u},
    {8,0,0x37,15,0x8A,1,2,0,0x800F6A58u,0x27BDFFE8u,0x800F9F1Cu,0x800F52D4u,0x800F53F8u,0xA062008Au},
    {9,0,0x3B,16,0x84,0,2,0,0x800EF928u,0x27BDFFE8u,0x800F1FF4u,0x800EE9A8u,0x800EEB68u,0xA2020084u},
};
typedef struct {
    Mmx5CoopDamageBank damage;
    Mmx5CoopCooldown cooldown;
    uint32_t controller,vulnerable_table;
    uint8_t descriptor,window,window_phase;
} BossBalanceState;
static BossBalanceState boss_balance[48];
static uint32_t boss_generations[48];
static unsigned boss_scaling,boss_per_player;
static unsigned balance_current,balance_call,balance_contact_call,balance_draw_call;
static uint32_t balance_pending_actor,balance_pending_function;
static unsigned balance_raw,balance_owner,balance_prepared,balance_attempted;
static uint32_t balance_admitted_table;
static Mmx5CoopDamageHit balance_hit;

static int balance_slot(uint32_t actor) {
    if(actor<0x80092090u || actor>=0x80093DD0u || (actor-0x80092090u)%0x9Cu)return -1;
    return (int)((actor-0x80092090u)/0x9Cu);
}
static int balance_ram(uint32_t address) {
    return address>=0x80010000u && address<0x801FFF00u && !(address&3u);
}
static int balance_table(uint32_t address) {
    return address>=0x80010000u && address<0x801FFF00u && !(address&1u);
}
static int balance_descriptor_valid(const BossBalanceDescriptor *d,uint32_t actor,uint32_t root) {
    return (!root || root==d->root) && psx_mod_read_byte(PLAY)==10 &&
        psx_mod_read_byte(PLAY+12)==d->stage && psx_mod_read_byte(PLAY+13)==d->section &&
        psx_mod_read_word(0x800EE970u)==d->logical &&
        psx_mod_read_word(d->root)==d->root_word && psx_mod_read_word(d->table+4)==d->live &&
        psx_mod_read_word(0x80031F60u)==0x27BDFFE8u &&
        psx_mod_read_byte(actor+1)==d->cls && !psx_mod_read_byte(actor+2);
}
static unsigned balance_descriptor(uint32_t actor,uint32_t root) {
    if(balance_slot(actor)<0)return 0;
    for(unsigned i=0;i<9;++i)
        if(balance_descriptor_valid(&balance_descriptors[i],actor,root))return i+1;
    return 0;
}
static uint32_t balance_timer_address(const BossBalanceState *s,uint32_t actor) {
    const BossBalanceDescriptor *d=&balance_descriptors[s->descriptor-1u];
    return (d->controller?s->controller:actor)+d->timer_offset;
}
static unsigned balance_timer(const BossBalanceState *s,uint32_t actor) {
    uint32_t address=balance_timer_address(s,actor);
    return s->descriptor==4?psx_mod_read_half(address):psx_mod_read_byte(address);
}
static void balance_write_timer(const BossBalanceState *s,uint32_t actor,unsigned ticks) {
    uint32_t address=balance_timer_address(s,actor);
    if(s->descriptor==4)psx_mod_write_half(address,(uint16_t)ticks);
    else psx_mod_write_byte(address,(uint8_t)ticks);
}
static void balance_reset(void) {
    memset(boss_balance,0,sizeof boss_balance);memset(boss_generations,0,sizeof boss_generations);
    balance_current=balance_call=balance_contact_call=balance_draw_call=0;
    balance_pending_actor=balance_pending_function=0;
    balance_raw=balance_owner=balance_prepared=balance_attempted=0;balance_admitted_table=0;
    memset(&balance_hit,0,sizeof balance_hit);
}
static void balance_forget(uint32_t actor) {
    int i=balance_slot(actor);if(!boss_scaling || i<0)return;
    ++boss_generations[i];memset(&boss_balance[i],0,sizeof boss_balance[i]);
}
static int balance_begin(uint32_t actor,uint32_t root) {
    if(!boss_scaling || !enrolled || failed || balance_current || psx_mod_local_view_scope())return 0;
    unsigned id=balance_descriptor(actor,root);if(!id)return 0;
    unsigned i=(unsigned)balance_slot(actor);BossBalanceState *s=&boss_balance[i];
    if(!psx_mod_read_byte(actor) || !psx_mod_read_byte(actor+4) ||
       !psx_mod_read_byte(actor+0x5C) || psx_mod_read_byte(actor+0x5C)>127) {
        memset(s,0,sizeof *s);return 0;
    }
    /* PLAY20 is the real native campaign boss HP bank, not a child/clone. */
    if(psx_mod_read_word(PLAY+0x20)!=actor)return 0;
    uint32_t controller=balance_descriptors[id-1u].controller?
        psx_mod_read_word(SCRATCH+0x4C):0;
    if(controller && !balance_ram(controller))return 0;
    if(balance_descriptors[id-1u].controller && !controller)return 0;
    uint32_t bank=controller?controller:actor;
    if(s->descriptor!=id || s->controller!=controller || s->damage.actor!=actor ||
       s->damage.generation!=boss_generations[i] || s->damage.bank!=bank) {
        memset(s,0,sizeof *s);s->descriptor=(uint8_t)id;s->controller=controller;
        mmx5_coop_damage_bind(&s->damage,actor,boss_generations[i],bank);
    }
    balance_current=i+1u;balance_raw=balance_owner=balance_prepared=balance_attempted=0;
    balance_admitted_table=0;
    if(boss_per_player)mmx5_coop_cooldown_tick(&s->cooldown);
    return 1;
}
static void balance_end(void) {
    if(!balance_current)return;
    BossBalanceState *s=&boss_balance[balance_current-1u];uint32_t actor=s->damage.actor;
    if(!psx_mod_read_byte(actor) || !psx_mod_read_byte(actor+4) ||
       !psx_mod_read_byte(actor+0x5C))memset(s,0,sizeof *s);
    else {
        unsigned timer=balance_timer(s,actor);
        /* Do not carry ordinary recovery into an authored protected phase. */
        if(!timer || timer>15 || psx_mod_read_word(actor+0x58)!=BOSS_REJECTION_TABLE)s->window=0;
        if(s->window)s->window_phase=psx_mod_read_byte(actor+5);
    }
    balance_current=balance_raw=balance_owner=balance_prepared=balance_attempted=0;
    balance_admitted_table=0;memset(&balance_hit,0,sizeof balance_hit);
}
static void balance_redirect(CPUState *cpu,uint32_t address) {
    if(address!=0x80022044u || !boss_scaling || !enrolled || failed ||
       balance_call || balance_current || balance_pending_actor || psx_mod_local_view_scope())return;
    uint32_t actor=cpu->gpr[4],root=cpu->gpr[5];
    if(!balance_descriptor(actor,root))return;
    balance_pending_actor=actor;balance_pending_function=root;cpu->gpr[5]=BALANCE_DISPATCH;
}
static void balance_update(CPUState *cpu,uint32_t address) {
    (void)address;uint32_t actor=balance_pending_actor,root=balance_pending_function;
    balance_pending_actor=balance_pending_function=0;
    if(!actor || !root || balance_call)return;
    balance_call=1;int tracked=balance_begin(actor,root);
    uint32_t result=guest(cpu,root,actor,root);
    if(tracked)balance_end();
    balance_call=0;cpu->gpr[2]=result;
}
static void balance_damage(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!boss_scaling || !balance_current || !balance_contact_call || balance_prepared ||
       psx_mod_local_view_scope())return;
    BossBalanceState *s=&boss_balance[balance_current-1u];uint32_t actor=cpu->gpr[7];
    if(actor!=s->damage.actor)return;
    unsigned character=psx_mod_read_byte(PLAYER+2);
    if(!mmx5_coop_damage_prepare(&s->damage,psx_mod_read_byte(actor+0x5C),
                                cpu->gpr[6],character,&balance_hit))return;
    balance_admitted_table=psx_mod_read_word(actor+0x58);
    cpu->gpr[2]=balance_hit.hp_after;balance_prepared=1;
}
/* Frozen X4 gates its low-level accepted overlap, not the outer scanner:
 * retain the actual shot and held token, while an empty overlap scan must
 * still clear the seat's old token. X5's native duplicate7E has that exact
 * boundary before31F98/31FA0 token stores and32030 shot disposition. */
static int balance_private_contact(CPUState *cpu,uint32_t address) {
    if(address!=0x80031F60u || !boss_scaling || !boss_per_player ||
       !balance_current || !balance_contact_call || !balance_attempted ||
       balance_attempted>2 || psx_mod_local_view_scope())return 0;
    BossBalanceState *s=&boss_balance[balance_current-1u];
    if(cpu->gpr[4]!=s->damage.actor ||
       mmx5_coop_cooldown_ready(&s->cooldown,balance_attempted-1u))return 0;
    cpu->gpr[2]=0x7Eu;return 1;
}
static uint32_t balance_contact(CPUState *cpu,uint32_t address,uint32_t actor,uint32_t argument,unsigned seat) {
    if(address!=0x80031DD0u || !boss_scaling || !balance_current || balance_contact_call ||
       psx_mod_local_view_scope())return guest(cpu,address,actor,argument);
    BossBalanceState *s=&boss_balance[balance_current-1u];
    if(s->damage.actor!=actor)return guest(cpu,address,actor,argument);
    int blocked=boss_per_player && !mmx5_coop_cooldown_ready(&s->cooldown,seat);
    uint32_t old_table=psx_mod_read_word(actor+0x58);unsigned old_timer=balance_timer(s,actor);
    /* Only the retained ordinary 1->0 boundary may expose the ready seat.
     * 7A, native tokens and every other authored protection remain native. */
    int opened=!blocked && boss_per_player && s->window && old_timer==1 &&
        s->window_phase==psx_mod_read_byte(actor+5) && old_table==BOSS_REJECTION_TABLE &&
        /* First Dynamo explicitly masks contact during frames5A..5D before
         * its special timer store; that protection precedes this callback. */
        !(s->descriptor==9 && psx_mod_read_byte(actor+0x47)>=0x5A &&
                             psx_mod_read_byte(actor+0x47)<=0x5D) &&
        balance_table(s->vulnerable_table) && s->vulnerable_table!=BOSS_REJECTION_TABLE;
    if(opened) {psx_mod_write_word(actor+0x58,s->vulnerable_table);balance_write_timer(s,actor,0);}
    balance_contact_call=1;balance_prepared=0;balance_attempted=seat+1u;
    uint32_t result=guest(cpu,address,actor,argument);
    if(balance_prepared) {
        /* This includes accepted raw damage with zero integer HP loss. */
        if(!mmx5_coop_damage_commit(&s->damage,&balance_hit))failed=1;
        balance_raw+=balance_hit.native_amount;balance_owner=seat+1u;
        if(balance_table(balance_admitted_table) && balance_admitted_table!=BOSS_REJECTION_TABLE)
            s->vulnerable_table=balance_admitted_table;
    }else if(opened) {
        psx_mod_write_word(actor+0x58,old_table);balance_write_timer(s,actor,old_timer);
    }
    balance_contact_call=balance_prepared=balance_attempted=0;
    /* The actual duplicate gate maps to0 in31DD0, preserving ready-partner
     * fallback and leaving all original native caller reactions untouched. */
    return blocked?0:result;
}
static void balance_recovery(CPUState *cpu,uint32_t address) {
    if(!boss_scaling || !balance_current || !balance_raw || !balance_owner ||
       psx_mod_local_view_scope())return;
    BossBalanceState *s=&boss_balance[balance_current-1u];
    const BossBalanceDescriptor *d=&balance_descriptors[s->descriptor-1u];
    if(address!=d->install || psx_mod_read_word(address)!=d->install_word)return;
    uint32_t instruction=d->install_word;
    unsigned base=(instruction>>21)&31u,reg=(instruction>>16)&31u;
    if(cpu->gpr[base]+(instruction&65535u)!=balance_timer_address(s,s->damage.actor))return;
    unsigned native=cpu->gpr[reg];
    if(native!=2 && native!=30)return;
    cpu->gpr[reg]=mmx5_coop_cooldown_install(&s->cooldown,balance_owner-1u,native,boss_per_player);
    s->window=(uint8_t)(boss_per_player && s->vulnerable_table!=0);
}
static void balance_phase_timer(CPUState *cpu,uint32_t address) {
    (void)cpu;
    if(!boss_scaling || !balance_current || psx_mod_local_view_scope())return;
    BossBalanceState *s=&boss_balance[balance_current-1u];
    if((s->descriptor==4 && address==0x800F7EF4u) ||
       (s->descriptor==9 && (address==0x800EEAA4u || address==0x800EEAD0u || address==0x800EEA7Cu)))
        s->window=0;
}
static void balance_reaction(CPUState *cpu,uint32_t address) {
    if(!boss_scaling || !balance_current || !balance_raw || psx_mod_local_view_scope())return;
    BossBalanceState *s=&boss_balance[balance_current-1u];
    if(s->descriptor==2 && address==0x800F34CCu && cpu->gpr[17]==s->damage.actor)
        cpu->gpr[16]=balance_raw; /* S1 is actor; S0 is raw HP loss here. */
    else if(s->descriptor==8 && address==0x800F80D0u &&
            cpu->gpr[16]==s->damage.actor && cpu->gpr[3]==cpu->gpr[2])
        /* Only this comparison consumes v1; the original fallthrough replaces
         * it before further use. Preserve HP and native reaction writes. */
        cpu->gpr[3]=cpu->gpr[2]^0x100u;
}
static int balance_draw(CPUState *cpu,uint32_t address) {
    if(address!=0x80024C60u || !boss_scaling || !boss_per_player || balance_draw_call ||
       !enrolled || failed)return 0;
    uint32_t actor=cpu->gpr[4];int i=balance_slot(actor);if(i<0)return 0;
    BossBalanceState *s=&boss_balance[i];
    if(!s->descriptor || s->damage.actor!=actor || psx_mod_read_byte(actor+4)!=1 ||
       !balance_descriptor_valid(&balance_descriptors[s->descriptor-1u],actor,0))return 0;
    /* Ordinary private flash must not replace an authored protected-phase
     * palette (class34's10000 and first Dynamo's40/120/frame5A..5D). */
    if(balance_timer(s,actor)>15 ||
       (s->descriptor==9 && psx_mod_read_byte(actor+0x47)>=0x5A &&
                            psx_mod_read_byte(actor+0x47)<=0x5D))return 0;
    unsigned ticks=s->cooldown.ticks[0]>s->cooldown.ticks[1]?s->cooldown.ticks[0]:s->cooldown.ticks[1];
    if(!ticks)return 0;
    const BossBalanceDescriptor *d=&balance_descriptors[s->descriptor-1u];
    uint16_t color=psx_mod_read_half(actor+0x42),draw=color&0x7FFFu;
    if((!!(ticks&d->flash_mask))!=d->flash_zero)draw|=0x8000u;
    balance_draw_call=1;psx_mod_write_half(actor+0x42,draw);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    psx_mod_write_half(actor+0x42,color);balance_draw_call=0;cpu->gpr[2]=result;return 1;
}
