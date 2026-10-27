#include "mmx5_coop_lifecycle.h"
#include "mmx5_coop_scene_anchor.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static void mounted_return_rules(void) {
    CHECK(mmx5_coop_native_incoming(1,0,255,0,0));
    CHECK(!mmx5_coop_native_incoming(1,2,0,0,0));
    CHECK(!mmx5_coop_native_incoming(1,0x12,1,255,0));
    CHECK(!mmx5_coop_native_incoming(1,0,255,0,1));
    CHECK(!mmx5_coop_native_incoming(2,0,255,0,0));
    Mmx5CoopMountedReturn rider={0,1,0,1,0x12,1,255};
    Mmx5CoopArmorSurface rides[2]={{96u<<16,352u<<16,0x800FDE84u,0x41,1,1,8,0x40},{0}};
    Mmx5CoopReturnAnchor out={0};
    CHECK(mmx5_coop_armor_return_anchor(&rider,rides,&out));
    CHECK(out.x==96u<<16 && out.y==352u<<16 && out.box==0x800FDE84u);
    /* Native vehicle coordinates remain authoritative while the rider's
     * body follows the preceding update; do not require an idle pose. */
    rides[0].x+=32768;CHECK(mmx5_coop_armor_return_anchor(&rider,rides,&out));
    CHECK(out.x==(96u<<16)+32768);
    rides[0].ground=0;CHECK(!mmx5_coop_armor_return_anchor(&rider,rides,&out));
    rides[0].ground=8;rides[0].flags=0;CHECK(!mmx5_coop_armor_return_anchor(&rider,rides,&out));
    rides[0].flags=0x40;rides[0].kind=0;CHECK(!mmx5_coop_armor_return_anchor(&rider,rides,&out));
    rides[0].kind=1;rides[0].state=2;CHECK(!mmx5_coop_armor_return_anchor(&rider,rides,&out));
    rides[0].state=1;rides[0].box=0;CHECK(!mmx5_coop_armor_return_anchor(&rider,rides,&out));
    rides[0].box=0x800FDE84u;rider.locked=1;CHECK(!mmx5_coop_armor_return_anchor(&rider,rides,&out));
    rider.locked=0;rider.owner=1;CHECK(!mmx5_coop_armor_return_anchor(&rider,rides,&out));
    rider.owner=0;rider.native_overlay=0;CHECK(!mmx5_coop_armor_return_anchor(&rider,rides,&out));
    rider.native_overlay=1;rides[1]=rides[0];CHECK(!mmx5_coop_armor_return_anchor(&rider,rides,&out));
    CHECK(mmx5_coop_return_probe_safe(8,0,27,27));
    CHECK(mmx5_coop_return_probe_safe(9,0,27,27));
    CHECK(!mmx5_coop_return_probe_safe(0,0,27,27));
    CHECK(!mmx5_coop_return_probe_safe(12,0,27,27));
    CHECK(!mmx5_coop_return_probe_safe(11,0,27,27));
    CHECK(!mmx5_coop_return_probe_safe(8,1,27,27));
    CHECK(!mmx5_coop_return_probe_safe(8,0,27,128));
    CHECK(!mmx5_coop_return_probe_safe(8,0,0,0));
    CHECK(!mmx5_coop_return_probe_safe(8,0,128,128));
    CHECK(mmx5_coop_return_probe_safe(8,0,0x9B,0x9B)); /* Preserve pending native HP flags. */
}
int main(void) {
    mounted_return_rules();
    Mmx5CoopLifecycle s;
    mmx5_coop_lifecycle_init(&s);
    /* Holding Select at boot does not count as a deliberate departure. */
    for (int n=0;n<100;++n) CHECK(mmx5_coop_join_input(&s,1,1,1)==MMX5_COOP_NO_ACTION);
    mmx5_coop_join_input(&s,0,1,1);
    for (int n=0;n<89;++n) CHECK(mmx5_coop_join_input(&s,1,1,1)==MMX5_COOP_NO_ACTION);
    CHECK(mmx5_coop_join_input(&s,1,1,1)==MMX5_COOP_LEAVE);
    CHECK(s.status[1]==MMX5_COOP_LEAVING);
    mmx5_coop_teleport_done(&s);
    CHECK(s.status[1]==MMX5_COOP_ABSENT);
    CHECK(mmx5_coop_join_input(&s,1,1,1)==MMX5_COOP_NO_ACTION);
    mmx5_coop_join_input(&s,0,1,1);
    CHECK(mmx5_coop_join_input(&s,1,1,0)==MMX5_COOP_NO_ACTION);
    mmx5_coop_join_input(&s,0,1,1);
    CHECK(mmx5_coop_join_input(&s,1,1,1)==MMX5_COOP_JOIN);
    mmx5_coop_teleport_done(&s);
    CHECK(s.status[1]==MMX5_COOP_ALIVE);
    mmx5_coop_join_input(&s,0,1,1);
    for (int n=0;n<60;++n) mmx5_coop_join_input(&s,1,1,1);
    mmx5_coop_join_input(&s,1,0,1); /* Pause/scene breaks consecutive gameplay hold. */
    for (int n=0;n<89;++n) CHECK(mmx5_coop_join_input(&s,1,1,1)==MMX5_COOP_NO_ACTION);
    CHECK(mmx5_coop_join_input(&s,1,1,1)==MMX5_COOP_LEAVE);
    mmx5_coop_lifecycle_respawn(&s);
    mmx5_coop_fatal(&s,0);
    CHECK(!s.wipe && mmx5_coop_living(&s,1));
    mmx5_coop_join_input(&s,0,1,1);
    for (int n=0;n<100;++n) CHECK(mmx5_coop_join_input(&s,1,1,1)==MMX5_COOP_NO_ACTION);
    mmx5_coop_fatal(&s,1);
    CHECK(s.wipe==2);
    mmx5_coop_lifecycle_respawn(&s);
    mmx5_coop_fatal(&s,1);
    s.status[1]=MMX5_COOP_FALLEN;
    mmx5_coop_join_input(&s,0,1,1);
    CHECK(mmx5_coop_join_input(&s,1,1,1)==MMX5_COOP_NO_ACTION);
    CHECK(!s.wipe);
    mmx5_coop_fatal(&s,0);
    CHECK(s.wipe==1);
    mmx5_coop_lifecycle_respawn(&s);
    CHECK(s.status[0]==MMX5_COOP_ALIVE && s.status[1]==MMX5_COOP_ALIVE && !s.wipe);
    CHECK(mmx5_coop_limit_x(600*65536,100*65536,320)==380*65536);
    CHECK(mmx5_coop_limit_x(0,400*65536,320)==120*65536);
    CHECK(mmx5_coop_limit_x(500*65536,100*65536,640)==500*65536);
    CHECK(mmx5_coop_zero_available(4,0,0));
    CHECK(!mmx5_coop_zero_available(5,0,0));
    CHECK(mmx5_coop_zero_available(5,0x80,0));
    CHECK(mmx5_coop_zero_available(5,0,1));
    CHECK(mmx5_coop_car_sequence(5,0,0)); /* No car yet: still the forced sequence. */
    CHECK(mmx5_coop_car_sequence(5,1,1)); /* Checkpoint advanced while still riding. */
    CHECK(!mmx5_coop_car_sequence(5,1,0));
    CHECK(!mmx5_coop_car_sequence(4,0,1));
    mmx5_coop_lifecycle_respawn(&s);
    mmx5_coop_join_input(&s,0,1,1);
    for(int n=0;n<180;++n)
        CHECK(mmx5_coop_join_input(&s,1,!mmx5_coop_car_sequence(5,0,0),1)==MMX5_COOP_NO_ACTION);
    CHECK(s.status[1]==MMX5_COOP_ALIVE && !s.select_frames);
    mmx5_coop_fatal(&s,0);CHECK(!s.wipe && mmx5_coop_living(&s,1));
    mmx5_coop_fatal(&s,1);CHECK(s.wipe==2);
    mmx5_coop_lifecycle_respawn(&s);s.status[1]=MMX5_COOP_UNAVAILABLE;
    mmx5_coop_join_input(&s,0,1,1);
    CHECK(!mmx5_coop_living(&s,1));
    CHECK(mmx5_coop_join_input(&s,1,1,1)==MMX5_COOP_NO_ACTION);
    mmx5_coop_fatal(&s,0);CHECK(s.wipe==1);
    puts("co-op lifecycle rules PASS");
    return 0;
}
