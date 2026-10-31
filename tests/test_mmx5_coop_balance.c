/* Execute the title's actual hook/transaction implementation against a small
 * deterministic native-call fixture. This proves integration protocol, not
 * collision/overlay/gameplay qualification of an original boss. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mmx5_coop_damage.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {uint32_t gpr[32];} CPUState;
#define PLAY 0x800D1C00u
#define PLAYER 0x8009A0A0u
#define SCRATCH 0x1F800000u
static unsigned enrolled=1,failed, boss_calls,contact_calls,reject,raw=1,overlap=1,lowlevel_calls;
static uint8_t ram[0x200000],scratch[0x400];
static uint8_t *memory(uint32_t a) {
    if(a>=SCRATCH && a<SCRATCH+sizeof scratch)return scratch+(a-SCRATCH);
    CHECK(a>=0x80000000u && a<0x80200000u);return ram+(a-0x80000000u);
}
static uint8_t psx_mod_read_byte(uint32_t a){return *memory(a);}
static uint16_t psx_mod_read_half(uint32_t a){return psx_mod_read_byte(a)|(uint16_t)psx_mod_read_byte(a+1)<<8;}
static uint32_t psx_mod_read_word(uint32_t a){return psx_mod_read_half(a)|(uint32_t)psx_mod_read_half(a+2)<<16;}
static void psx_mod_write_byte(uint32_t a,uint8_t v){*memory(a)=v;}
static void psx_mod_write_half(uint32_t a,uint16_t v){psx_mod_write_byte(a,(uint8_t)v);psx_mod_write_byte(a+1,(uint8_t)(v>>8));}
static void psx_mod_write_word(uint32_t a,uint32_t v){psx_mod_write_half(a,(uint16_t)v);psx_mod_write_half(a+2,(uint16_t)(v>>16));}
static int psx_mod_local_view_scope(void){return 0;}
static uint32_t guest(CPUState *,uint32_t,uint32_t,uint32_t);
#include "../src/mods/mmx5_coop_balance.inc"

static uint32_t guest(CPUState *cpu,uint32_t address,uint32_t actor,uint32_t argument) {
    (void)argument;
    if(address!=0x80031DD0u){++boss_calls;return 0;}
    ++contact_calls;
    /* Original31DF4/31DFC invulnerability and31F38 no-contact reset precede
     * any low-level hit.31ECC overlap decides whether31F60 is called. */
    if(psx_mod_read_byte(actor+0x7A) || !overlap) {
        psx_mod_write_byte(actor+0x65,0);return 0;
    }
    CPUState native=*cpu;native.gpr[4]=actor;native.gpr[5]=0x80098120;
    ++lowlevel_calls;
    if(balance_private_contact(&native,0x80031F60u)) {
        CHECK(native.gpr[2]==0x7E);return 0; /*31EF0 maps duplicate7E to0. */
    }
    /* These stand in for native31F98 and reaction8's32030. The title gate
     * must happen before either store, never by skipping the outer scan. */
    psx_mod_write_byte(actor+0x65,psx_mod_read_byte(0x80098120+0x64));
    if(reject || psx_mod_read_word(actor+0x58)==BOSS_REJECTION_TABLE) {
        psx_mod_write_byte(0x80098120+0x98,1);return 0x7F;
    }
    native.gpr[7]=actor;native.gpr[6]=raw;
    native.gpr[2]=psx_mod_read_byte(actor+0x5C)-raw;
    balance_damage(&native,0x80032184u);
    psx_mod_write_byte(actor+0x5C,(uint8_t)native.gpr[2]);
    return native.gpr[2]?1u:UINT32_MAX;
}
static uint32_t setup(unsigned id) {
    balance_reset();boss_scaling=boss_per_player=1;reject=0;raw=overlap=1;
    const BossBalanceDescriptor *d=&balance_descriptors[id-1u];uint32_t actor=0x80092090u;
    memset(ram,0,sizeof ram);memset(scratch,0,sizeof scratch);
    psx_mod_write_byte(PLAY,10);psx_mod_write_byte(PLAY+12,d->stage);psx_mod_write_byte(PLAY+13,d->section);
    psx_mod_write_word(0x80031F60,0x27BDFFE8);
    psx_mod_write_word(0x800EE970,d->logical);psx_mod_write_word(d->root,d->root_word);
    psx_mod_write_word(d->table+4,d->live);psx_mod_write_word(d->install,d->install_word);
    psx_mod_write_word(PLAY+0x20,actor);psx_mod_write_byte(actor,1);
    psx_mod_write_byte(actor+1,d->cls);psx_mod_write_byte(actor+4,1);psx_mod_write_byte(actor+5,3);
    psx_mod_write_byte(actor+0x5C,64);psx_mod_write_word(actor+0x58,0x80075C00);
    if(d->controller)psx_mod_write_word(SCRATCH+0x4C,0x80180000);
    return actor;
}
static void install(CPUState *cpu,unsigned ticks) {
    BossBalanceState *s=&boss_balance[balance_current-1u];
    const BossBalanceDescriptor *d=&balance_descriptors[s->descriptor-1u];
    unsigned base=(d->install_word>>21)&31u,reg=(d->install_word>>16)&31u;
    cpu->gpr[base]=balance_timer_address(s,s->damage.actor)-d->timer_offset;cpu->gpr[reg]=ticks;
    balance_recovery(cpu,d->install);
    balance_write_timer(s,s->damage.actor,cpu->gpr[reg]);
    psx_mod_write_word(s->damage.actor+0x58,BOSS_REJECTION_TABLE);
}
int main(void) {
    CPUState cpu={{0}};uint32_t actor=setup(1),root=balance_descriptors[0].root;
    CHECK(balance_begin(actor,root));
    CHECK(balance_contact(&cpu,0x80031DD0,actor,0,0)==1);
    CHECK(psx_mod_read_byte(actor+0x5C)==64 && boss_balance[0].damage.remainder==5);
    install(&cpu,30);CHECK(balance_timer(&boss_balance[0],actor)==1 && boss_balance[0].cooldown.ticks[0]==15);
    balance_end();CHECK(boss_balance[0].window);
    CHECK(balance_begin(actor,root));unsigned calls=contact_calls;
    uint32_t serial=boss_balance[0].damage.serial;
    psx_mod_write_byte(actor+0x65,0x45);psx_mod_write_byte(0x80098120+0x64,0x46);
    psx_mod_write_byte(0x80098120+0x98,0);
    CHECK(!balance_contact(&cpu,0x80031DD0,actor,0,0) && contact_calls==calls+1);
    CHECK(psx_mod_read_byte(actor+0x65)==0x45 && !psx_mod_read_byte(0x80098120+0x98));
    CHECK(boss_balance[0].damage.serial==serial && boss_balance[0].damage.remainder==5);
    CHECK(balance_timer(&boss_balance[0],actor)==1 &&
          psx_mod_read_word(actor+0x58)==BOSS_REJECTION_TABLE && !balance_attempted);
    /* Empty overlap must clear this seat's old key, without consuming the
     * shot or crediting damage. An outer31DD0 skip failed this condition. */
    overlap=0;unsigned low=lowlevel_calls;
    CHECK(!balance_contact(&cpu,0x80031DD0,actor,0,0));
    CHECK(contact_calls==calls+2 && lowlevel_calls==low && !psx_mod_read_byte(actor+0x65));
    CHECK(!psx_mod_read_byte(0x80098120+0x98) && boss_balance[0].damage.serial==serial);
    overlap=1;psx_mod_write_byte(actor+0x7A,1);psx_mod_write_byte(actor+0x65,0x45);
    CHECK(!balance_contact(&cpu,0x80031DD0,actor,0,0) && lowlevel_calls==low);
    CHECK(!psx_mod_read_byte(actor+0x65));psx_mod_write_byte(actor+0x7A,0);
    reject=1;CHECK(balance_contact(&cpu,0x80031DD0,actor,0,1)==0x7F);
    CHECK(balance_timer(&boss_balance[0],actor)==1 && psx_mod_read_word(actor+0x58)==BOSS_REJECTION_TABLE);
    CHECK(boss_balance[0].damage.serial==serial && boss_balance[0].damage.remainder==5);
    CHECK(psx_mod_read_byte(0x80098120+0x98)==1); /* Ready seat keeps native rejection. */
    reject=0;psx_mod_write_byte(PLAYER+2,1);
    CHECK(balance_contact(&cpu,0x80031DD0,actor,0,1)==1);
    CHECK(boss_balance[0].damage.remainder==9 && balance_timer(&boss_balance[0],actor)==0);
    /* Native caller sees timer0 after accepted zero-loss contact, before its
     * own reaction/recovery logic. A third commit in one update is ordered. */
    CHECK(balance_contact(&cpu,0x80031DD0,actor,0,1)==1);
    CHECK(psx_mod_read_byte(actor+0x5C)==63 && boss_balance[0].damage.remainder==3);
    install(&cpu,2);CHECK(boss_balance[0].cooldown.ticks[1]==1);balance_end();
    uint8_t before=boss_balance[0].cooldown.ticks[0];
    balance_pending_actor=actor;balance_pending_function=root;balance_update(&cpu,BALANCE_DISPATCH);
    CHECK(boss_calls==1 && boss_balance[0].cooldown.ticks[0]==before-1);
    /* Lifetime and protected nonzero-action continuity. */
    psx_mod_write_byte(actor+4,3);CHECK(balance_begin(actor,root));
    CHECK(boss_balance[0].damage.remainder==3);balance_end();
    balance_forget(actor);CHECK(boss_generations[0]==1 && !boss_balance[0].descriptor);
    actor=setup(2);CHECK(balance_begin(actor,balance_descriptors[1].root));
    raw=3;CHECK(balance_contact(&cpu,0x80031DD0,actor,0,0)==1);
    cpu.gpr[17]=actor;cpu.gpr[16]=1;balance_reaction(&cpu,0x800F34CC);
    CHECK(cpu.gpr[16]==3);balance_end();
    actor=setup(8);CHECK(balance_begin(actor,balance_descriptors[7].root));
    CHECK(balance_contact(&cpu,0x80031DD0,actor,0,0)==1);
    cpu.gpr[16]=actor;cpu.gpr[2]=cpu.gpr[3]=64;balance_reaction(&cpu,0x800F80D0);
    CHECK(cpu.gpr[2]==64 && cpu.gpr[3]!=64);balance_end();
    actor=setup(9);CHECK(balance_begin(actor,balance_descriptors[8].root));
    CHECK(balance_contact(&cpu,0x80031DD0,actor,0,0)==1);install(&cpu,30);balance_end();
    psx_mod_write_byte(actor+0x47,0x5A);CHECK(balance_begin(actor,balance_descriptors[8].root));
    CHECK(balance_contact(&cpu,0x80031DD0,actor,0,1)==0x7F);
    CHECK(balance_timer(&boss_balance[0],actor)==1);
    balance_phase_timer(&cpu,0x800EEAA4);CHECK(!boss_balance[0].window);balance_end();
    puts("X5 balance accepted-zero/rejected rollback/once-only recovery/lifetime/reaction contracts PASS");
    return 0;
}
