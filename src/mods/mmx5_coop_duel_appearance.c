#include "mmx5_coop_duel_appearance.h"

int mmx5_coop_duel_x_appearance(unsigned native_visual_group,
                              Mmx5CoopDuelXAppearance *out) {
    Mmx5CoopDuelXAppearance plan;
    if (!out || (native_visual_group != 5 && native_visual_group != 19))
        return 0;
    plan.record = 93;
    plan.graphics_group = native_visual_group == 5 ? 0 : 10;
    plan.assembly_group = native_visual_group == 5 ? 0 : 18;
    plan.armor_group = (uint8_t)native_visual_group;
    plan.draw_roster = 4;
    *out = plan;
    return 1;
}

int mmx5_coop_duel_palette_offset(uint16_t actor_clut, unsigned rows,
                                 size_t palette_bytes, size_t *byte_offset) {
    unsigned clut = actor_clut & 0x7FFFu;
    unsigned y = clut >> 6;
    unsigned x_slot = clut & 63u;
    size_t offset, bytes;
    if (!byte_offset || y < 480 || y >= 512 || x_slot >= 16 ||
        !rows || rows > 16 - x_slot)
        return 0;
    offset = ((size_t)(y - 480) * 16 + x_slot) * 32;
    bytes = (size_t)rows * 32;
    if (offset > palette_bytes || bytes > palette_bytes - offset)
        return 0;
    *byte_offset = offset;
    return 1;
}

int mmx5_coop_duel_x_palette_row(uint16_t draw_clut, unsigned *npc_row) {
    unsigned row;
    if (!npc_row) return 0;
    if (draw_clut == 0x7800u) row = 0;
    else if (draw_clut == 0x7843u) row = 1;
    else return 0;
    *npc_row = row;
    return 1;
}
