#ifndef MMX5_COOP_DUEL_APPEARANCE_H
#define MMX5_COOP_DUEL_APPEARANCE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Presentation only. Keep the original body frame, position, animation clock,
 * abilities, shots and menu state. DAT93's base plus native Ultimate armor
 * composition reproduces the original NPC X standing art; DAT88/Fourth does
 * not. This plan does not equip Ultimate or change a simulation resource. */
typedef struct {
    uint8_t record;
    uint8_t graphics_group;
    uint8_t assembly_group;
    uint8_t armor_group;
    uint8_t draw_roster;
} Mmx5CoopDuelXAppearance;

/* Native 3B33C/3B4CC cache the action family in BODY+155: 5 is ordinary,
 * 19 alternate. The selected frame BODY+47 stays unchanged. Draw base using
 * assembly_group, then native 26CAC with armor_group (the FA4 composition).
 * Use a separate immutable DAT93 draw bank when simulation uses DAT88.
 * Failure leaves out unchanged; no guest state is read or written. */
int mmx5_coop_duel_x_appearance(unsigned native_visual_group,
                              Mmx5CoopDuelXAppearance *out);

/* Resolve an actor CLUT into the original SCRATCH+28 palette buffer. Native
 * RECT 800703CC is (0,480,256,32): 16 CLUTs per row, not 64. The actor's
 * high bit is its native flash flag; resolve its underlying palette here.
 * Consecutive requested CLUTs must remain within that physical row.
 * Failure leaves byte_offset unchanged. */
int mmx5_coop_duel_palette_offset(uint16_t actor_clut, unsigned rows,
                                 size_t palette_bytes, size_t *byte_offset);

/* Standing source proof maps player body CLUT7800 to NPC relative row0 and
 * Ultimate armor CLUT7843 to NPC row1. 7801 is a weapon-body palette;
 * 7844 is a parts/program armor palette. Their NPC mapping is unverified,
 * so return failure for them and for native flash/effect CLUTs. This is a
 * draw-bank color lookup, never permission to rewrite live player palettes. */
int mmx5_coop_duel_x_palette_row(uint16_t draw_clut, unsigned *npc_row);

#ifdef __cplusplus
}
#endif
#endif
