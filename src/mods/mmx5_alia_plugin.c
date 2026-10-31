#include "mod_plugins.h"
#include "cpu_state.h"
#include "mmx5_coop_internal.h"

#define ID "mmx5.skip-alia"

/* SLUS-01334: retain category3/type14's native admission and heard bit.
 * 56BB4 tests the actual hint rectangle and records the heard bit; Grizzly's
 * F71E8 queries that bit before enabling cannon damage and train scripts.
 * Deleting an unstarted hint loses those actions even in single player.
 * After56D54 records that bit, suppress only this hint's stand command and
 * select its native retirement state. No lock, freeze or gather is needed
 * for a silent completed hint. Cannon/timer scripts still see native knowledge.
 * Other dialogue, including capsules and story scenes, remains native.
 */
static int skip_hint_hold(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=cpu->gpr[17];
    if(cpu->gpr[31]!=0x80056D64u || cpu->gpr[4]!=0x14u ||
       psx_mod_read_word(0x80056D54u)!=0xA0A20000u ||
       psx_mod_read_word(0x80056D5Cu)!=0x0C00F4A7u ||
       psx_mod_read_word(0x80056D64u)!=0x92230004u ||
       actor<0x8009E7A0u || actor>=0x8009F9A0u || (actor-0x8009E7A0u)%0x30u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=14u ||
       psx_mod_read_byte(actor+2)>=54u || psx_mod_read_byte(actor+4))return 0;
    /* Native56D64 increments this to2, whose56F00 retires the admitted actor.
     * The heard bit has already been committed by original56D54. Never mark
     * an unseen/out-of-range hint or directly enable a cannon/timer here. */
    psx_mod_write_byte(actor+4,1);
    mmx5_coop_silent_hint();
    cpu->gpr[2]=0;
    psx_mod_counter_add("mmx5.skip-alia.completed-without-hold",1);
    return 1;
}
static int skip_hint_text(CPUState *cpu, uint32_t address) {
    (void)address;
    /* BIN7's post-bike orb tutorial is authored inside its stage controller,
     * rather than category3/type14. Suppress only those three Alia requests;
     * the original controller still creates the orb, issues shoot command1F,
     * waits for its real completion and releases control through3D2C4. */
    uint32_t caller=cpu->gpr[31];
    if(psx_mod_read_byte(0x800D1C0Cu)==5 && !psx_mod_read_byte(0x800D1C0Du) &&
       psx_mod_read_word(0x800EE970u)==11 &&
       ((caller==0x800F1058u && cpu->gpr[4]==0x37u) ||
        (caller==0x800F1130u && cpu->gpr[4]==0x38u) ||
        (caller==0x800F1144u && cpu->gpr[4]==0x39u)) &&
       psx_mod_read_word(0x800F1050u)==0x0C008E17u &&
       psx_mod_read_word(0x800F1128u)==0x0C008E17u &&
       psx_mod_read_word(0x800F113Cu)==0x0C008E17u &&
       psx_mod_read_word(0x800F119Cu)==0x0C00F4B1u) {
        cpu->gpr[2]=0;
        psx_mod_counter_add("mmx5.skip-alia.orb-tutorial-text",1);
        return 1;
    }
    if (cpu->gpr[31] != 0x80056e4cu ||
        psx_mod_read_word(0x80056e44u) != 0x0c008e17u ||
        psx_mod_read_word(0x80056e4cu) != 0x92430005u ||
        psx_mod_read_word(0x80056f78u) != 0x0c00f4b1u)
        return 0;
    cpu->gpr[2] = 0;
    psx_mod_counter_add("mmx5.skip-alia.suppressed-text", 1);
    return 1;
}

PSX_MOD_CONSTRUCTOR(mmx5_register_alia_plugin) {
    psx_mod_register_function_filter_plugin(ID,0x8003D29C,skip_hint_hold);
    psx_mod_register_function_filter_plugin(ID, 0x8002385cu, skip_hint_text);
    /* The trusted online profile clears offline package preferences. Hint
     * suppression is fixed on for both peers, including a fresh install.
     * CMake hashes this source and the package into that profile's identity.
     */
    psx_game_register_netplay_function_filter(0x8002385cu, skip_hint_text);
    psx_game_register_netplay_function_filter(0x8003D29C,skip_hint_hold);
}
