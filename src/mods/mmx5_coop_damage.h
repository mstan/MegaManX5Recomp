#ifndef MMX5_COOP_DAMAGE_H
#define MMX5_COOP_DAMAGE_H
#include <stdint.h>

/* Fractional arithmetic from frozen X4 mmx4_coop_balance.c:308..329:
 * X credits 5/10 and Zero 4/10 of accepted native damage. One remainder is
 * shared by both attackers for each native boss health bank.
 *
 * This module neither identifies bosses nor changes native admission,
 * attack IDs, weakness reactions, recovery timers, HP or progression.
 * The caller enables it only for the optional co-op boss_scaling mode.
 */
typedef struct {
    uint32_t actor, generation, bank, serial;
    uint8_t remainder; /* Tenths of HP, 0..9. Hash/serialize fields, not padding. */
} Mmx5CoopDamageBank;

typedef struct {
    uint32_t actor, generation, bank, serial, committed_serial;
    uint16_t native_amount, scaled_amount;
    uint8_t remainder_before, remainder_after, hp_before, hp_after, state;
} Mmx5CoopDamageHit;

/* Initialize once at activation. Keep one bank per actual native health
 * pool; switching pools must not overwrite another pool's retained carry. */
void mmx5_coop_damage_init(Mmx5CoopDamageBank *bank);
/* Bind to a deterministic allocation generation, never a host pointer or
 * frame counter. A changed actor/generation/bank discards the old carry.
 * actor zero is reserved for an unbound bank. */
int mmx5_coop_damage_bind(Mmx5CoopDamageBank *bank, uint32_t actor,
                         uint32_t generation, uint32_t health_bank);
void mmx5_coop_damage_forget(Mmx5CoopDamageBank *bank);

/* Prepare only at a positive native damage subtraction, after native
 * collision/token/table checks and native damage modifiers.
 *
 * Original SLUS-01334 80032180 subtracts GPR6 (a2) from HP. At instruction 80032184
 * (34638000), GPR7 (a3) is the boss, GPR5 (a1) the actual shot, GPR6 (a2)
 * the final native amount, and GPR2 (v0) the native post-subtraction HP.
 * Replace only that HP result with hit.hp_after. Keep GPR6, actor+63, shot IDs and the native lethal/return path.
 * X5 permits a legitimate full boss meter of 127; high-bit HP is excluded.
 *
 * Preparation does not change the bank. The caller must determine actual
 * native acceptance, including accepted hits whose integer scaled loss is
 * zero. HP movement alone cannot decide acceptance. Misses and rejected or
 * duplicate contacts must not commit. zero_attack is the actual accepted
 * attacker (0=X, 1=Zero), not P1's campaign or the current AI target.
 */
int mmx5_coop_damage_prepare(const Mmx5CoopDamageBank *bank, unsigned hp,
                            unsigned native_amount, unsigned zero_attack,
                            Mmx5CoopDamageHit *hit);
/* Commit only after the native caller has accepted this hit. A stale or
 * repeated proposal is rejected. A committed lethal hit clears the carry,
 * matching frozen X4 balance_end's zero-HP cleanup. */
int mmx5_coop_damage_commit(Mmx5CoopDamageBank *bank, Mmx5CoopDamageHit *hit);
int mmx5_coop_damage_discard(Mmx5CoopDamageHit *hit);
/* Some native boss callers restore HP after the generic damage function.
 * Frozen X4 rejected_hit (376..387) restores its pre-update fraction too.
 * If such a caller rejects a provisionally committed hit, undo it BEFORE
 * another hit is committed to this bank. Undo affects carry only; the native
 * caller owns restoring HP and contact/reaction state. A stale/duplicate
 * undo, including one across actor reuse, cannot overwrite later carry.
 *
 * If several tentative hits can occur before a caller's acceptance decision,
 * the integration must retain a transaction for that whole native update;
 * do not treat this latest-hit undo API as a multi-hit rollback journal.
 */
int mmx5_coop_damage_undo(Mmx5CoopDamageBank *bank, Mmx5CoopDamageHit *hit);

/* Integration must reset on allocation/new health-bank lifetime and include
 * all bank fields in deterministic state and future snapshot/view handling.
 * Do not mutate them in presentation-only passes. These helpers provide no
 * savestate support or cooldown policy by themselves.
 */
/* Frozen X4 ordinary recovery policy, effective only with scaling enabled.
 * One tick belongs to ONE real native boss update, never to each contact,
 * view, player projection or render. Installs accept only native2/30.
 * Shared mode returns ceil(native/2). Per-player mode records the actual
 * accepted seat, then returns min(private timers), with a floor of1 so the
 * original 1->0 table/flash cleanup remains reachable. Rejected contacts do
 * not install or consume recovery. The title integration owns protected
 * phases and temporary table/timer reopening for a ready partner. */
typedef struct {uint8_t ticks[2];} Mmx5CoopCooldown;
void mmx5_coop_cooldown_tick(Mmx5CoopCooldown *cooldown);
int mmx5_coop_cooldown_ready(const Mmx5CoopCooldown *cooldown,unsigned seat);
unsigned mmx5_coop_cooldown_install(Mmx5CoopCooldown *cooldown,unsigned seat,
                                    unsigned native_ticks,unsigned per_player);
#endif
