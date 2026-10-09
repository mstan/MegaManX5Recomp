#ifndef MMX5_COOP_LIFECYCLE_H
#define MMX5_COOP_LIFECYCLE_H
#include <stdint.h>

typedef enum {
    MMX5_COOP_ALIVE, MMX5_COOP_LEAVING, MMX5_COOP_ABSENT,
    MMX5_COOP_JOINING, MMX5_COOP_DYING, MMX5_COOP_FALLEN, MMX5_COOP_UNAVAILABLE
} Mmx5CoopStatus;
typedef enum { MMX5_COOP_NO_ACTION, MMX5_COOP_LEAVE, MMX5_COOP_JOIN } Mmx5CoopJoinAction;
typedef struct {
    Mmx5CoopStatus status[2];
    unsigned select_frames;
    uint8_t select_armed, select_previous, wipe; /* wipe = last fallen seat + 1 */
} Mmx5CoopLifecycle;

void mmx5_coop_lifecycle_init(Mmx5CoopLifecycle *s);
int mmx5_coop_living(const Mmx5CoopLifecycle *s, unsigned player);
/* Called on native death entry, before the death animation finishes. */
void mmx5_coop_fatal(Mmx5CoopLifecycle *s, unsigned player);
/* Only a new stage or a team respawn may clear FALLEN. An ordinary door may not. */
void mmx5_coop_lifecycle_respawn(Mmx5CoopLifecycle *s);
Mmx5CoopJoinAction mmx5_coop_join_input(Mmx5CoopLifecycle *s,
                                     int select, int gameplay, int safe_landing);
/* Completion is separate from the request, so native teleport art can finish. */
void mmx5_coop_teleport_done(Mmx5CoopLifecycle *s);
int32_t mmx5_coop_limit_x(int32_t x, int32_t other, unsigned viewport_width);
/* Original BIN member 20, 800EEE90..800EEED4: roster entry 5 is Zero. */
int mmx5_coop_zero_available(unsigned campaign_phase,unsigned zero_flags,int keep_playable);
/* Native stage5/checkpoint0 forces the Ride Chaser sequence (80020968..84).
 * Keep voluntary departure locked through rider death/mount transitions. */
int mmx5_coop_car_sequence(unsigned stage,unsigned checkpoint,int chaser_active);
#endif
