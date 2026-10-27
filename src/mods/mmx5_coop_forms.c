/* SLUS-01334 automatic native forms. Options affect selected native reads;
 * PLAY+4A, genuine capsule/code grants and native card serialization stay real.
 * Ultimate resource/body ownership is supplied by the co-op integration. */
#include "mmx5_coop_forms.h"
#include "mod_plugins.h"
#include "mod_runtime.h"
#include "mod_netplay.h"
#include "mmx5_coop_internal.h"
#include "cpu_state.h"
#include <stdint.h>

#define PLAYER 0x8009A0A0u
#define PLAY 0x800D1C00u
#define OVERLAY_HEADER 0x800EE970u
#define BLACK_BIT 0x10u
#define BLACK_MASK 0x30420010u
#define ULTIMATE_PACKAGE "mmx5.cheat.ultimate-armor"
#define BLACK_PACKAGE "mmx5.cheat.black-zero"
#define COOP_PACKAGE "mmx5.coop"

static unsigned coop_choices[2];

/* Native New Game ties Z-Buster to PLAY1BA, which also records the starting
 * campaign hero in native saves. Co-op grants the ability at its three
 * consumers; never overwrite that persistent campaign field. */
static void coop_z_buster(CPUState *cpu,uint32_t address) {
    if(!cpu || !mod_runtime_snapshot_package_active(COOP_PACKAGE) ||
       !psx_mod_read_byte(PLAYER) || psx_mod_read_byte(PLAYER+2)!=1)return;
    address=0x80000000u|(address&0x1FFFFFFFu);
    unsigned reg=0;
    if(address==0x80018A4Cu && psx_mod_read_word(address-8)==0x922201BAu &&
       cpu->gpr[17]==PLAY && psx_mod_read_word(address)==0x1440000Du)reg=2;
    if(address==0x80033334u && psx_mod_read_word(address-8)==0x906401BAu &&
       cpu->gpr[3]==PLAY && psx_mod_read_word(address)==0x10800004u)reg=4;
    if(address==0x80041B04u && psx_mod_read_word(address-8)==0x90431DBAu &&
       cpu->gpr[2]==0x800D0000u && psx_mod_read_word(address)==0x1060001Bu)reg=3;
    if(reg && !psx_mod_read_word(address-4)) {
        cpu->gpr[reg]=1;
        psx_mod_counter_add("mmx5.coop.z-buster-grants",1);
    }
}

void mmx5_coop_forms_configure(int ultimate_x, int black_zero) {
    coop_choices[0] = ultimate_x != 0;
    coop_choices[1] = black_zero != 0;
}

unsigned mmx5_coop_forms_enabled(unsigned character) {
    if (character > 1) return 0;
    if(character==1 && mod_runtime_snapshot_package_active(COOP_PACKAGE))
        return mmx5_coop_zero_form();
    unsigned agreed = mod_runtime_snapshot_package_active(COOP_PACKAGE) &&
                      coop_choices[character];
    /* A stale offline selection cannot affect a synchronized profile. The
     * committed package gate also makes old co-op choices inert when off. */
    if (psx_mod_netplay_is_active()) return agreed;
    return agreed || mod_runtime_snapshot_package_active(
        character ? BLACK_PACKAGE : ULTIMATE_PACKAGE);
}

unsigned mmx5_coop_forms_roster(unsigned character, unsigned native_roster) {
    /* Native New Game selects Fourth Armor1, not base X0. This explicit
     * cheat equips Ultimate for any native X armor selection; disabling it
     * restores the genuine selection without changing progress bytes. */
    return character == 0 && native_roster <= 4 &&
           mmx5_coop_forms_enabled(0) ? 4u : native_roster;
}

/* Targets below32 identify the native body register; zero means the fixed
 * PLAYER read used by UI. Frontend previews use selected roster5 even when
 * there is no live Zero body. All mask sites consume a loaded byte in v0. */
enum { TARGET_PLAYER = 0, TARGET_SELECTED_ROSTER = 32 };
typedef struct {
    uint32_t mask, load_word;
    uint8_t target, logical, stage;
} BlackRead;

static const BlackRead black_reads[] = {
    {0x8001850Cu, 0x9062004Au, TARGET_PLAYER, 0, 0}, /* Native UI */
    {0x8003176Cu, 0x9062004Au, 4, 0, 0},  /* Damage amount: a0 */
    {0x80031BC4u, 0x9062004Au, 16, 0, 0}, /* Hurt direction: s0 */
    {0x80031E5Cu, 0x90621C4Au, 5, 0, 0},  /* Contact: a1 */
    {0x80038E48u, 0x90621C4Au, 16, 0, 0}, /* Hurt motion: s0 */
    {0x8003C724u, 0x90621C4Au, 16, 0, 0}, /* Palette: s0 */
    {0x8003E75Cu, 0x90621C4Au, 6, 0, 0},  /* Native cost: a2 */
    {0x8004B7ECu, 0x90621C4Au, 18, 0, 0}, /* Owned attack: s2 */
    {0x80052EE8u, 0x90621C4Au, 6, 0, 0},  /* Actor property: a2 */
    {0x80055074u, 0x90621C4Au, 5, 0, 0},  /* Capsule admission: a1 */
    {0x800F1EF0u, 0x90A2004Au, TARGET_SELECTED_ROSTER, 24, 13},
    {0x800F4058u, 0x9082004Au, TARGET_SELECTED_ROSTER, 24, 13},
    {0x800F41A8u, 0x9082004Au, TARGET_SELECTED_ROSTER, 24, 13},
    {0x800F1510u, 0x90621C4Au, 6, 30, 16},
    {0x800F46F8u, 0x90621C4Au, 6, 31, 17},
    {0x800F1C18u, 0x90621C4Au, 6, 32, 18}
};

static int same_bus_address(uint32_t first, uint32_t second) {
    return (first & 0x1FFFFFFFu) == (second & 0x1FFFFFFFu);
}

static int zero_context(CPUState *cpu, const BlackRead *read) {
    if (read->target == TARGET_SELECTED_ROSTER)
        return psx_mod_read_byte(PLAY + 0x2A) == 5;
    /* HUD, ride and solid-contact scopes can project PLAYER without changing
     * the outer co-op projected boolean. Qualify the actual native body and
     * its source register, never the campaign or that outer boolean. */
    if (read->target && !same_bus_address(cpu->gpr[read->target], PLAYER))
        return 0;
    return psx_mod_read_byte(PLAYER) && psx_mod_read_byte(PLAYER + 2) == 1;
}

static void black_read(CPUState *cpu, uint32_t address) {
    int coop=mod_runtime_snapshot_package_active(COOP_PACKAGE);
    if (!cpu || (!coop && !mmx5_coop_forms_enabled(1))) return;
    address = 0x80000000u | (address & 0x1FFFFFFFu);
    for (unsigned i = 0; i < sizeof black_reads / sizeof black_reads[0]; ++i) {
        const BlackRead *read = &black_reads[i];
        if (address != read->mask) continue;
        /* Capsule admission consumes collected progress, not the currently
         * equipped Normal/Black choice. Choosing Normal cannot respawn an
         * already collected secret capsule. */
        if(coop && address==0x80055074u)return;
        if (psx_mod_read_word(address) != BLACK_MASK ||
            psx_mod_read_word(address - 8) != read->load_word ||
            psx_mod_read_word(address - 4) != 0) return;
        if(read->logical) {
            if(psx_mod_read_word(OVERLAY_HEADER)!=read->logical)return;
            /* The selector keeps BIN24 while PLAY+0C already names the
             * chosen mission. It is no longer the loader's stage13. */
            if(read->logical==24) {if(psx_mod_read_byte(PLAY)!=4)return;}
            else if(psx_mod_read_byte(PLAY+0x0C)!=read->stage ||
                    psx_mod_read_byte(PLAY+0x0D))return;
        }
        unsigned base = (read->load_word >> 21) & 31u;
        int32_t offset = (int16_t)(read->load_word & 65535u);
        if (!same_bus_address(cpu->gpr[base] + (uint32_t)offset, PLAY + 0x4A) ||
            !zero_context(cpu, read)) return;
        /* Execute the original ANDI and its following branch. Only this
         * loaded result changes; even read-modify-write55D8C and Save1D0C4
         * must read genuine RAM. The operation is idempotent when several
         * independently active packages register this same callback.
         *
         * The four actor initializers qualify their actual native PLAYER
         * source. Allocating-seat/deferred scheduling for those producers
         * still needs runtime qualification; no owner history is invented. */
        if(mmx5_coop_forms_enabled(1))cpu->gpr[2] |= BLACK_BIT;
        else cpu->gpr[2] &= ~BLACK_BIT;
        return;
    }
}


enum { TARGET_CAMPAIGN = 33 };
typedef struct {
    uint32_t site,load_word,delay_word,site_word;
    uint8_t result,target,logical;
} UltimateRead;

static const UltimateRead ultimate_reads[] = {
    {0x80013480u, 0x90430049u, 0x24020004u, 0x14620003u, 3, TARGET_CAMPAIGN, 0},
    {0x8003be78u, 0x904b0049u, 0x24030003u, 0x1563001au, 11, 17, 0},
    {0x8003c2bcu, 0x90a40049u, 0x00000000u, 0xa20400fau, 4, 16, 0},
    {0x8003d5f8u, 0x90c50049u, 0x00804021u, 0xa10500fau, 5, 8, 0},
    {0x8003d7acu, 0x90c50049u, 0x00808021u, 0xa20500fau, 5, 16, 0},
    {0x800f4080u, 0x8045002au, 0x0c005c7au, 0x02002021u, 5, TARGET_SELECTED_ROSTER, 24},
    {0x800f1f8cu, 0x8046002au, 0x0c005c85u, 0x24050002u, 6, TARGET_SELECTED_ROSTER, 24},
    {0x800f1b40u, 0x90441c49u, 0x24030003u, 0x10830003u, 4, TARGET_PLAYER, 12},
    {0x800f961cu, 0x90441c49u, 0x24030003u, 0x1483004cu, 4, TARGET_PLAYER, 31},
    {0x800f96a4u, 0x90441c49u, 0x24030003u, 0x1483002au, 4, TARGET_PLAYER, 31},
    {0x800f9734u, 0x90441c49u, 0x24030003u, 0x14830006u, 4, TARGET_PLAYER, 31},
    {0x800fb568u, 0x90441c49u, 0x24030003u, 0x14830003u, 4, TARGET_PLAYER, 31},
    {0x800fb710u, 0x90441c49u, 0x24030003u, 0x14830003u, 4, TARGET_PLAYER, 31},
    {0x800fb774u, 0x90441c49u, 0x24030003u, 0x10830016u, 4, TARGET_PLAYER, 31},
};

static void ultimate_read(CPUState *cpu,uint32_t address) {
    if(!cpu || !mmx5_coop_forms_enabled(0))return;
    address=0x80000000u|(address&0x1FFFFFFFu);
    for(unsigned i=0;i<sizeof ultimate_reads/sizeof ultimate_reads[0];++i) {
        const UltimateRead *read=&ultimate_reads[i];
        if(address!=read->site)continue;
        if(psx_mod_read_word(address)!=read->site_word ||
           psx_mod_read_word(address-8)!=read->load_word ||
           psx_mod_read_word(address-4)!=read->delay_word)return;
        if(read->logical && psx_mod_read_word(OVERLAY_HEADER)!=read->logical)return;
        if(read->target==TARGET_SELECTED_ROSTER && psx_mod_read_byte(PLAY)!=4)return;
        if(read->target==TARGET_CAMPAIGN) {
            /* 1347C has replaced the load's base register. These original
             * resident instructions establish PLAY before that exact load. */
            if(psx_mod_read_byte(PLAY+0x44) ||
               psx_mod_read_word(0x8001345C)!=0x3C02800Du ||
               psx_mod_read_word(0x80013460)!=0x24421C00u)return;
        } else {
            unsigned base=(read->load_word>>21)&31u;
            int32_t offset=(int16_t)(read->load_word&65535u);
            uint32_t source=PLAY+(read->target==TARGET_SELECTED_ROSTER?0x2Au:0x49u);
            if(!same_bus_address(cpu->gpr[base]+(uint32_t)offset,source))return;
            if(read->target==TARGET_SELECTED_ROSTER) {
                if(psx_mod_read_byte(PLAY+0x2A)>4)return;
            } else if((read->target && !same_bus_address(cpu->gpr[read->target],PLAYER)) ||
                      psx_mod_read_byte(PLAYER+2) ||
                      (!read->target && !psx_mod_read_byte(PLAYER)))return;
        }
        /* Body/parts/resource selection and the authored Ultimate preview
         * consume the same effective roster. Native selection/unlock/card
         * bytes stay genuine. Option-off manual codes and armor choices
         * remain native. Stage Gaea checks consume this same effective
         * roster when the automatic option overrides a Gaea selection. */
        cpu->gpr[read->result]=mmx5_coop_forms_roster(0,cpu->gpr[read->result]);
        return;
    }
}

PSX_MOD_CONSTRUCTOR(mmx5_register_coop_forms) {
    psx_mod_register_instruction_plugin("mmx5.coop",0x80018A4Cu,0x1440000Du,coop_z_buster);
    psx_mod_register_instruction_plugin("mmx5.coop",0x80033334u,0x10800004u,coop_z_buster);
    psx_mod_register_instruction_plugin("mmx5.coop",0x80041B04u,0x1060001Bu,coop_z_buster);
    static const char *const plugins[] = {
        "mmx5.cheats.ultimate-armor", "mmx5.cheats.black-zero", "mmx5.coop"
    };
    for (unsigned p = 0; p < sizeof plugins / sizeof plugins[0]; ++p) {
        for (unsigned i = 0; i < sizeof black_reads / sizeof black_reads[0]; ++i)
            psx_mod_register_instruction_plugin(plugins[p], black_reads[i].mask,
                                                BLACK_MASK, black_read);
        for (unsigned i = 0; i < sizeof ultimate_reads / sizeof ultimate_reads[0]; ++i)
            psx_mod_register_instruction_plugin(plugins[p], ultimate_reads[i].site,
                                                ultimate_reads[i].site_word, ultimate_read);
    }
}
