#include "mmx5_coop_damage.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

static unsigned accept(Mmx5CoopDamageBank *bank, unsigned hp, unsigned raw, unsigned zero) {
    Mmx5CoopDamageHit hit;
    CHECK(mmx5_coop_damage_prepare(bank, hp, raw, zero, &hit));
    CHECK(mmx5_coop_damage_commit(bank, &hit));
    return hit.hp_after;
}

int main(void) {
    Mmx5CoopDamageBank bank, other;
    Mmx5CoopDamageHit x, zero, stale;
    unsigned hp;

    mmx5_coop_damage_init(&bank);
    CHECK(!mmx5_coop_damage_prepare(&bank, 64, 1, 1, &zero));
    CHECK(mmx5_coop_damage_bind(&bank, 0x80092090u, 1, 0));
    /* Accepted sub-HP contacts retain credit; misses/rejections do not. */
    CHECK(mmx5_coop_damage_prepare(&bank, 64, 1, 1, &zero));
    CHECK(zero.hp_after == 64 && zero.remainder_after == 4 && bank.remainder == 0);
    CHECK(mmx5_coop_damage_discard(&zero));
    CHECK(!mmx5_coop_damage_commit(&bank, &zero) && bank.remainder == 0);

    /* One common carry joins actual X/Zero hits, including zero visible loss. */
    hp = accept(&bank, 64, 1, 0);
    CHECK(hp == 64 && bank.remainder == 5);
    hp = accept(&bank, hp, 1, 1);
    CHECK(hp == 64 && bank.remainder == 9);
    hp = accept(&bank, hp, 1, 1);
    CHECK(hp == 63 && bank.remainder == 3);

    /* A native caller undo restores the fraction, not just its integer HP. */
    CHECK(mmx5_coop_damage_prepare(&bank, hp, 2, 1, &zero));
    CHECK(mmx5_coop_damage_commit(&bank, &zero));
    CHECK(zero.hp_after == 62 && bank.remainder == 1);
    CHECK(!mmx5_coop_damage_commit(&bank, &zero));
    CHECK(!mmx5_coop_damage_discard(&zero));
    CHECK(mmx5_coop_damage_undo(&bank, &zero) && bank.remainder == 3);
    CHECK(!mmx5_coop_damage_undo(&bank, &zero));

    /* Later accepted damage cannot be overwritten by a stale undo/proposal. */
    CHECK(mmx5_coop_damage_prepare(&bank, hp, 1, 0, &x));
    CHECK(mmx5_coop_damage_prepare(&bank, hp, 1, 1, &stale));
    CHECK(mmx5_coop_damage_commit(&bank, &x));
    CHECK(!mmx5_coop_damage_commit(&bank, &stale));
    hp = accept(&bank, hp, 1, 1);
    CHECK(!mmx5_coop_damage_undo(&bank, &x));
    CHECK(bank.remainder == 2);

    /* Frozen X4's 36 raw Zero damage credits 14.4 HP across small hits. */
    CHECK(mmx5_coop_damage_bind(&bank, 0x80092090u, 2, 0));
    hp = 64;
    for (unsigned i = 0; i < 36; ++i) hp = accept(&bank, hp, 1, 1);
    CHECK(hp == 50 && bank.remainder == 4);
    CHECK(mmx5_coop_damage_bind(&bank, 0x80092090u, 2, 0));
    CHECK(bank.remainder == 4); /* Same live bank is not a reset. */

    /* Other health pools retain their own carry; reused slots start fresh. */
    mmx5_coop_damage_init(&other);
    CHECK(mmx5_coop_damage_bind(&other, 0x80092090u, 2, 1));
    CHECK(accept(&other, 64, 1, 0) == 64 && other.remainder == 5);
    CHECK(bank.remainder == 4);
    CHECK(mmx5_coop_damage_prepare(&bank, 50, 1, 1, &stale));
    CHECK(mmx5_coop_damage_bind(&bank, 0x80092090u, 3, 0));
    CHECK(!mmx5_coop_damage_commit(&bank, &stale) && !bank.remainder);

    /* Native X5 HP127 is valid. A lethal accepted hit clears residual carry,
     * and its native rejection can still restore the prior carry exactly. */
    CHECK(accept(&bank, 127, 1, 0) == 127 && bank.remainder == 5);
    CHECK(accept(&bank, 127, 1, 1) == 127 && bank.remainder == 9);
    CHECK(mmx5_coop_damage_prepare(&bank, 1, 3, 1, &zero));
    CHECK(zero.hp_after == 0 && zero.scaled_amount == 2);
    CHECK(mmx5_coop_damage_commit(&bank, &zero) && bank.remainder == 0);
    CHECK(mmx5_coop_damage_undo(&bank, &zero) && bank.remainder == 9);
    mmx5_coop_damage_forget(&bank);
    CHECK(!bank.actor && !bank.remainder);
    CHECK(!mmx5_coop_damage_undo(&bank, &zero));
    CHECK(!mmx5_coop_damage_bind(&bank, 0, 4, 0));
    CHECK(mmx5_coop_damage_bind(&bank, 0x80092090u, 4, 0));
    CHECK(!mmx5_coop_damage_prepare(&bank, 0, 1, 1, &zero));
    CHECK(!mmx5_coop_damage_prepare(&bank, 128, 1, 1, &zero));
    CHECK(!mmx5_coop_damage_prepare(&bank, 64, 0, 1, &zero));
    CHECK(!mmx5_coop_damage_prepare(&bank, 64, 256, 1, &zero));
    CHECK(!mmx5_coop_damage_prepare(&bank, 64, 1, 2, &zero));


    Mmx5CoopCooldown recovery={{0,0}};
    CHECK(mmx5_coop_cooldown_ready(&recovery,0) && mmx5_coop_cooldown_ready(&recovery,1));
    CHECK(!mmx5_coop_cooldown_ready(&recovery,2));
    CHECK(mmx5_coop_cooldown_install(&recovery,0,30,0)==15);
    CHECK(!recovery.ticks[0] && !recovery.ticks[1]); /* Shared has no private immunity. */
    CHECK(mmx5_coop_cooldown_install(&recovery,1,2,0)==1);
    CHECK(mmx5_coop_cooldown_install(&recovery,0,30,1)==1);
    CHECK(recovery.ticks[0]==15 && mmx5_coop_cooldown_ready(&recovery,1));
    /* Merely consulting another contact must not decrement either timer. */
    CHECK(!mmx5_coop_cooldown_ready(&recovery,0) && recovery.ticks[0]==15);
    CHECK(mmx5_coop_cooldown_install(&recovery,1,30,1)==15);
    mmx5_coop_cooldown_tick(&recovery);CHECK(recovery.ticks[0]==14 && recovery.ticks[1]==14);
    CHECK(mmx5_coop_cooldown_install(&recovery,1,2,1)==1);
    CHECK(recovery.ticks[0]==14 && recovery.ticks[1]==1);
    mmx5_coop_cooldown_tick(&recovery);
    CHECK(recovery.ticks[0]==13 && !recovery.ticks[1]);
    for(unsigned i=0;i<13;++i)mmx5_coop_cooldown_tick(&recovery);
    CHECK(!recovery.ticks[0] && !recovery.ticks[1]);
    mmx5_coop_cooldown_tick(&recovery);CHECK(!recovery.ticks[0]);
    /* Authored protection timers are outside this helper's ordinary2/30 policy. */
    CHECK(mmx5_coop_cooldown_install(&recovery,0,40,1)==40 && !recovery.ticks[0]);
    CHECK(mmx5_coop_cooldown_install(&recovery,2,30,1)==30 && !recovery.ticks[0]);
    mmx5_coop_cooldown_tick(NULL);
    puts("co-op accepted fractional damage and ordinary recovery rules PASS");
    return 0;
}
