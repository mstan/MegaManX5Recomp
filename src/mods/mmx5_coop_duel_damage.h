#ifndef MMX5_COOP_DUEL_DAMAGE_H
#define MMX5_COOP_DUEL_DAMAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MMX5_COOP_DUEL_BODY_BYTES 0x158u
#define MMX5_COOP_DUEL_PROXY_BYTES 0x9Cu

/* Initial 32-HP PvP profile, keyed by original attack class. Reserved actors,
 * shield/cast/visual-only objects do no damage. Native overlap, attack tokens
 * and player recovery still decide whether any amount is accepted. */
unsigned mmx5_coop_duel_attack_damage(unsigned attack_type);

/* Stateless host-buffer preparation only. This module never registers hooks,
 * executes guest code, writes guest memory or owns the duel/scene/menu state.
 * Source contract: SLUS-01334 31DD0/31F60, 32184, 317B8 and 2E274/2E408.
 *
 * Integration MUST enable this only for the qualified active story duel,
 * outside presentation/menu passes. Each direction uses the actual attacker's
 * private SHOTS and the other real PLAYER body. No body-touch or NPC attacks.
 */

/* Whether original317B8 can presently take an ordinary positive injury on
 * this on-foot body. Also gate the original global800C9318 at the call site.
 * 7A must be0: native7A=2 only admits attackers in80096D98..<80098118,
 * which excludes a temporary original-NPC incoming slot (as well as SHOTS).
 * Actual hit-box memory and native asset ownership remain the caller's job.
 */
int mmx5_coop_duel_damage_ready(
    const uint8_t defender[MMX5_COOP_DUEL_BODY_BYTES]);

/* Build an undrawn damage-only target for original31DD0. native_story_table
 * is the ORIGINAL live story NPC's proven normal +58, matched to the defender
 * character by the caller, never a guessed constant or a temporary rejection
 * table. The caller validates its complete native attack-index extent.
 *
 * Do not skip the scan merely because ready()==0. A valid but invulnerable
 * body produces proxy+61!=0, so native2E408 misses and31F38 clears the held
 * token just as in the original scanner. Capture proxy+65 after the scan.
 * The caller owns/reset/serializes that byte for each target's duel lifetime.
 * Returns0 for invalid/dead/mounted bodies or same-seat/self contact.
 * Output and source buffers must not overlap; failure leaves output intact.
 */
int mmx5_coop_duel_damage_target(
    uint8_t proxy[MMX5_COOP_DUEL_PROXY_BYTES],
    const uint8_t defender[MMX5_COOP_DUEL_BODY_BYTES],
    unsigned attacker_seat, unsigned defender_seat,
    uint32_t native_story_table, uint8_t native_token);

/* At original32184 BEFORE projectile consumption, GPR7 is the above target,
 * GPR5 is the actual attacker SHOTS slot and GPR6 is computed native damage.
 * Snapshot THAT shot before any player projection changes its guest bank.
 * This prepares a damage-only incoming actor for original317B8, preserving
 * the real attack box, facing and fractional position. Screen-relative shots
 * are normalized using the ATTACKER's native camera integer origins, as in
 * 2E408. No attack/collision/pose is invented. +62=0 selects ordinary native
 * player hurt rather than an NPC weakness-reaction/capture command.
 *
 * In an exact defender context call317B8 with this temporary incoming actor.
 * The duel-only31670 filter supplies the selected PvP amount unchanged;
 * ordinary non-duel injuries retain native armor/difficulty. Restore attacker context and
 * the damage target before resuming31F60. Use result() below to replace ONLY
 * GPR2 with actual remaining hero HP at32184. Keep GPR6 and the native
 * continuation:321EC then receives the correct lethal/nonlethal disposition.
 * Calling31DD0 to completion and applying injury later gives wrong projectile
 * consumption when armor changes lethality.
 *
 * Amount must be the positive signed native GPR6 result,1..127. Nonpositive
 * status/virus paths are not mapped by this ordinary-injury helper. Caller
 * supplies a genuine SHOTS snapshot, not a PLAYER body or autonomous NPC.
 * Failure leaves output intact; buffers must not overlap.
 */
int mmx5_coop_duel_damage_incoming(
    uint8_t incoming[MMX5_COOP_DUEL_PROXY_BYTES],
    const uint8_t shot[MMX5_COOP_DUEL_PROXY_BYTES],
    unsigned native_amount, int attacker_camera_x, int attacker_camera_y);

/* Validate an actual positive native317B8 injury, not an assumed HP delta.
 * Original player HP uses bit80 for pending injury and exactly80 for fatal.
 * On success remaining_hp is0..126 for the32184 native NPC continuation.
 * A failed native injury must not be turned into accepted proxy damage;
 * the integration must restore its pre-contact transaction or stop that
 * qualified path. This function does not perform rollback or modify a body.
 */
int mmx5_coop_duel_damage_result(
    const uint8_t before[MMX5_COOP_DUEL_BODY_BYTES],
    const uint8_t after[MMX5_COOP_DUEL_BODY_BYTES],
    uint32_t native_injury_result, unsigned *remaining_hp);

#ifdef __cplusplus
}
#endif
#endif
