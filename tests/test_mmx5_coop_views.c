#include "mmx5_coop_views.h"
#include <assert.h>
int main(void) {
    assert(!mmx5_coop_views_requested(0,"split"));
    assert(!mmx5_coop_views_requested(1,"unified"));
    assert(mmx5_coop_views_requested(1,"split"));
    int16_t cameras[2][2]={{0,0},{1200,800}};
    Mmx5CoopViewActor actors[2]={{160,120,1},{1360,920,1}};
    PSXPlacementRect regions[2];
    unsigned count=mmx5_coop_view_regions(cameras,actors,-1,regions);
    assert(count==2 && mmx5_coop_view_contains(regions,count,160,120));
    assert(mmx5_coop_view_contains(regions,count,1360,920));
    assert(!mmx5_coop_view_contains(regions,count,700,500));
    assert(!mmx5_coop_view_contains(regions,count,1360,120));
    count=mmx5_coop_view_regions(cameras,actors,1,regions);
    assert(count==1 && !mmx5_coop_view_contains(regions,count,160,120));
    actors[1].active=0;assert(mmx5_coop_view_nearest(actors,1360,920,1)==0);
    actors[1].active=1;
    assert(mmx5_coop_view_nearest(actors,1360,920,0)==1);
    actors[0]=(Mmx5CoopViewActor){-32768,-32768,1};
    actors[1]=(Mmx5CoopViewActor){32767,32767,1};
    assert(mmx5_coop_view_nearest(actors,32760,32760,0)==1);
    actors[0]=(Mmx5CoopViewActor){-10,0,1};actors[1]=(Mmx5CoopViewActor){10,0,1};
    assert(mmx5_coop_view_nearest(actors,0,0,0)==0);
    assert(mmx5_coop_view_nearest(actors,0,0,1)==1);
    return 0;
}
