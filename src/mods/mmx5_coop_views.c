#include "mmx5_coop_views.h"
#include <string.h>
int mmx5_coop_views_requested(int online,const char *option) {
    return online && option && !strcmp(option,"split");
}
unsigned mmx5_coop_view_nearest(const Mmx5CoopViewActor actors[2],int16_t x,int16_t y,unsigned preferred) {
    unsigned first=preferred<2?preferred:0,selected=first;
    uint64_t best=UINT64_MAX;
    for(unsigned order=0;order<2;++order) {
        unsigned seat=first^order;if(!actors[seat].active)continue;
        int64_t dx=(int64_t)actors[seat].x-x,dy=(int64_t)actors[seat].y-y;
        uint64_t distance=(uint64_t)(dx*dx+dy*dy);
        if(distance<best) {best=distance;selected=seat;}
    }
    return selected;
}
unsigned mmx5_coop_view_regions(const int16_t cameras[2][2],const Mmx5CoopViewActor actors[2],
                                int owner,PSXPlacementRect regions[2]) {
    unsigned count=0;
    /* Original 2B070/2B520: 48px activation apron, 320x240 viewport. */
    for(unsigned seat=0;seat<2;++seat)if(actors[seat].active && (owner<0 || owner==(int)seat))
        regions[count++]=(PSXPlacementRect){cameras[seat][0]-48,cameras[seat][0]+368,
                                           cameras[seat][1]-48,cameras[seat][1]+288};
    return count;
}
int mmx5_coop_view_contains(const PSXPlacementRect *regions,unsigned count,int x,int y) {
    for(unsigned index=0;index<count;++index)if(psx_placement_contains(regions[index],x,y))return 1;
    return 0;
}
