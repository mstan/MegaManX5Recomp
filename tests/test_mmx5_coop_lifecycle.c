#include "mmx5_coop_lifecycle.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
int main(void) {
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
