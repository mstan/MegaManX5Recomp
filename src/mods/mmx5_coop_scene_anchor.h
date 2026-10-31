#ifndef MMX5_COOP_SCENE_ANCHOR_H
#define MMX5_COOP_SCENE_ANCHOR_H
#include <stdint.h>

/* Original 3D72C starts action0 with personal67=FF. Ordinary idle, attacks,
 * mounted arrivals and scripts cannot enroll a second on-foot beam early. */
static inline int mmx5_coop_native_incoming(unsigned state,unsigned action,
        unsigned personal,unsigned mounted,unsigned locked) {
    return state==1 && !action && personal==255 && !mounted && !locked;
}

typedef struct {
    uint32_t x,y,box;
} Mmx5CoopReturnAnchor;
typedef struct {
    uint32_t x,y,box;
    uint8_t active,kind,state,ground,flags;
} Mmx5CoopArmorSurface;
typedef struct {
    unsigned owner,native_overlay,locked;
    uint8_t state,action,personal,mounted;
} Mmx5CoopMountedReturn;

/* Each co-op seat owns its physical Armor slot. Native39AC8 negates its
 * pending slot1/2 marker to FF/FE. The rider has no on-foot box; never borrow
 * the other occupied suit as a returning hero's floor. */
static inline int mmx5_coop_armor_return_anchor(const Mmx5CoopMountedReturn *rider,
        const Mmx5CoopArmorSurface rides[2],Mmx5CoopReturnAnchor *out) {
    if(!rider || !rides || !out || rider->owner>1 || !rider->native_overlay ||
       rider->locked || rider->state!=1 || rider->action!=0x12 ||
       rider->personal!=1 || rider->mounted!=255u-rider->owner)return 0;
    const Mmx5CoopArmorSurface *ride=&rides[rider->owner];
    if(!ride->active || !(ride->flags&0x40u) || ride->kind!=1 || ride->state!=1 || !(ride->ground&8u) ||
       ride->box<0x80010000u || ride->box>0x801FFFFCu)return 0;
    out->x=ride->x;out->y=ride->y;out->box=ride->box;
    return 1;
}

/* Native 2FB58 reports opposing terrain contacts, hazard79 and crush HP.
 * An Armor's own floor is insufficient: the incoming standing hero must fit. */
static inline int mmx5_coop_return_probe_safe(unsigned contact,unsigned hazard,
        unsigned hp_before,unsigned hp_after) {
    return (hp_before&127u) && (contact&8u) && (contact&12u)!=12u && (contact&3u)!=3u &&
        !hazard && hp_before==hp_after;
}
#endif
