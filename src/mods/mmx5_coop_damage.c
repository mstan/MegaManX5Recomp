#include "mmx5_coop_damage.h"
#include <string.h>

enum { HIT_EMPTY, HIT_PREPARED, HIT_COMMITTED, HIT_DISCARDED, HIT_UNDONE };

static int same_bank(const Mmx5CoopDamageBank *bank, const Mmx5CoopDamageHit *hit) {
    return bank && hit && bank->actor && bank->actor == hit->actor &&
        bank->generation == hit->generation && bank->bank == hit->bank;
}

void mmx5_coop_damage_init(Mmx5CoopDamageBank *bank) {
    if (bank) memset(bank, 0, sizeof *bank);
}

int mmx5_coop_damage_bind(Mmx5CoopDamageBank *bank, uint32_t actor,
                         uint32_t generation, uint32_t health_bank) {
    if (!bank || !actor) return 0;
    if (bank->actor != actor || bank->generation != generation || bank->bank != health_bank) {
        bank->actor = actor;
        bank->generation = generation;
        bank->bank = health_bank;
        bank->remainder = 0;
        ++bank->serial;
    }
    return 1;
}

void mmx5_coop_damage_forget(Mmx5CoopDamageBank *bank) {
    if (!bank) return;
    bank->actor = bank->generation = bank->bank = 0;
    bank->remainder = 0;
    ++bank->serial;
}

int mmx5_coop_damage_prepare(const Mmx5CoopDamageBank *bank, unsigned hp,
                            unsigned native_amount, unsigned zero_attack,
                            Mmx5CoopDamageHit *hit) {
    unsigned total, loss;
    if (!hit) return 0;
    memset(hit, 0, sizeof *hit);
    if (!bank || !bank->actor || bank->remainder > 9 || !hp || hp > 127 ||
        !native_amount || native_amount > 255 || zero_attack > 1) return 0;
    total = native_amount * (zero_attack ? 4u : 5u) + bank->remainder;
    loss = total / 10u;
    hit->actor = bank->actor;
    hit->generation = bank->generation;
    hit->bank = bank->bank;
    hit->serial = bank->serial;
    hit->native_amount = (uint16_t)native_amount;
    hit->scaled_amount = (uint16_t)loss;
    hit->hp_before = (uint8_t)hp;
    hit->hp_after = (uint8_t)(loss >= hp ? 0u : hp - loss);
    hit->remainder_before = bank->remainder;
    hit->remainder_after = (uint8_t)(loss >= hp ? 0u : total % 10u);
    hit->state = HIT_PREPARED;
    return 1;
}

int mmx5_coop_damage_commit(Mmx5CoopDamageBank *bank, Mmx5CoopDamageHit *hit) {
    if (!same_bank(bank, hit) || hit->state != HIT_PREPARED ||
        bank->serial != hit->serial || bank->remainder != hit->remainder_before)
        return 0;
    bank->remainder = hit->remainder_after;
    ++bank->serial;
    hit->committed_serial = bank->serial;
    hit->state = HIT_COMMITTED;
    return 1;
}

int mmx5_coop_damage_discard(Mmx5CoopDamageHit *hit) {
    if (!hit || hit->state != HIT_PREPARED) return 0;
    hit->state = HIT_DISCARDED;
    return 1;
}

int mmx5_coop_damage_undo(Mmx5CoopDamageBank *bank, Mmx5CoopDamageHit *hit) {
    if (!same_bank(bank, hit) || hit->state != HIT_COMMITTED ||
        bank->serial != hit->committed_serial || bank->remainder != hit->remainder_after)
        return 0;
    bank->remainder = hit->remainder_before;
    ++bank->serial;
    hit->state = HIT_UNDONE;
    return 1;
}

void mmx5_coop_cooldown_tick(Mmx5CoopCooldown *cooldown) {
    if(cooldown)for(unsigned i=0;i<2;++i)if(cooldown->ticks[i])--cooldown->ticks[i];
}
int mmx5_coop_cooldown_ready(const Mmx5CoopCooldown *cooldown,unsigned seat) {
    return cooldown && seat<2 && !cooldown->ticks[seat];
}
unsigned mmx5_coop_cooldown_install(Mmx5CoopCooldown *cooldown,unsigned seat,
                                    unsigned native_ticks,unsigned per_player) {
    if(!cooldown || seat>1 || (native_ticks!=2 && native_ticks!=30))return native_ticks;
    unsigned half=(native_ticks+1u)/2u;
    if(!per_player)return half;
    cooldown->ticks[seat]=(uint8_t)half;
    unsigned ticks=cooldown->ticks[0]<cooldown->ticks[1]?cooldown->ticks[0]:cooldown->ticks[1];
    return ticks?ticks:1u;
}
