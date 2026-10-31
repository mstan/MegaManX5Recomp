#ifndef MMX5_COOP_VIEWS_H
#define MMX5_COOP_VIEWS_H
#include "mod_visible_placements.h"
#include <stdint.h>
#define MMX5_COOP_PLACEMENTS 4096u
typedef struct {int16_t x,y;unsigned active;} Mmx5CoopViewActor;
int mmx5_coop_views_requested(int online,const char *option);
unsigned mmx5_coop_view_nearest(const Mmx5CoopViewActor actors[2],int16_t x,int16_t y,unsigned preferred);
unsigned mmx5_coop_view_regions(const int16_t cameras[2][2],const Mmx5CoopViewActor actors[2],
                                int owner,PSXPlacementRect regions[2]);
int mmx5_coop_view_contains(const PSXPlacementRect *regions,unsigned count,int x,int y);
#endif
