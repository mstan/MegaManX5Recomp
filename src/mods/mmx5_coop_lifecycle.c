#include "mmx5_coop_lifecycle.h"
#include <string.h>

void mmx5_coop_lifecycle_init(Mmx5CoopLifecycle *s) {
    memset(s, 0, sizeof *s);
    s->status[0] = s->status[1] = MMX5_COOP_ALIVE;
}
int mmx5_coop_living(const Mmx5CoopLifecycle *s, unsigned p) {
    return p < 2 && (s->status[p] == MMX5_COOP_ALIVE ||
                    s->status[p] == MMX5_COOP_JOINING || s->status[p] == MMX5_COOP_LEAVING);
}
void mmx5_coop_fatal(Mmx5CoopLifecycle *s, unsigned p) {
    if (p > 1 || s->status[p] == MMX5_COOP_FALLEN || s->status[p] == MMX5_COOP_DYING) return;
    s->status[p] = MMX5_COOP_DYING;
    if (!mmx5_coop_living(s, p ^ 1)) s->wipe = (uint8_t)(p+1);
    s->select_frames = 0;
}
void mmx5_coop_lifecycle_respawn(Mmx5CoopLifecycle *s) {
    mmx5_coop_lifecycle_init(s);
}
Mmx5CoopJoinAction mmx5_coop_join_input(Mmx5CoopLifecycle *s,
                                     int select, int gameplay, int safe_landing) {
    int pressed = select && !s->select_previous;
    s->select_previous = (uint8_t)!!select;
    if (!select) { s->select_armed = 1; s->select_frames = 0; }
    if (!gameplay || s->wipe) { s->select_frames = 0; return MMX5_COOP_NO_ACTION; }
    if (s->status[1] == MMX5_COOP_ALIVE && mmx5_coop_living(s, 0)) {
        if (select && s->select_armed && ++s->select_frames >= 90) {
            s->status[1] = MMX5_COOP_LEAVING;
            s->select_frames = s->select_armed = 0;
            return MMX5_COOP_LEAVE;
        }
    } else {
        s->select_frames = 0;
        if (s->status[1] == MMX5_COOP_ABSENT && pressed && safe_landing && mmx5_coop_living(s, 0)) {
            s->status[1] = MMX5_COOP_JOINING;
            s->select_armed = 0;
            return MMX5_COOP_JOIN;
        }
    }
    return MMX5_COOP_NO_ACTION;
}
void mmx5_coop_teleport_done(Mmx5CoopLifecycle *s) {
    if (s->status[1] == MMX5_COOP_LEAVING) s->status[1] = MMX5_COOP_ABSENT;
    else if (s->status[1] == MMX5_COOP_JOINING) s->status[1] = MMX5_COOP_ALIVE;
    s->select_frames = s->select_armed = 0;
}
int32_t mmx5_coop_limit_x(int32_t x, int32_t other, unsigned width) {
    /* Reserve one eighth of the shared viewport for body/sprite extents. */
    int64_t limit = (int64_t)width * 7 * 65536 / 8;
    int64_t delta = (int64_t)x - other;
    if (delta > limit) return (int32_t)((int64_t)other + limit);
    if (delta < -limit) return (int32_t)((int64_t)other - limit);
    return x;
}
int mmx5_coop_zero_available(unsigned phase,unsigned flags,int keep_playable) {
    return keep_playable || phase<5 || (flags&0x80);
}
int mmx5_coop_car_sequence(unsigned stage,unsigned checkpoint,int chaser_active) {
    return stage==5 && (!checkpoint || chaser_active);
}
