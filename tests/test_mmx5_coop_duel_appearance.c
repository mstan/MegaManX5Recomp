#include "mmx5_coop_duel_appearance.h"
#include <assert.h>
#include <string.h>

static void native_palette_rows(void) {
    /* Native2C338 writes contiguous 16-color slots;17484 uploads this as
     * RECT(0,480,256,32). Armor slots13/14 are on the second physical row,
     * unlike their encoded PSX CLUT deltas43/44. */
    uint16_t palette[256 * 32] = {0};
    static const struct { uint16_t clut, slot, color; } cases[] = {
        {0x7800, 0x00, 0x0100}, {0x780F, 0x0F, 0x0200},
        {0x7843, 0x13, 0x0300}, {0x7844, 0x14, 0x0400}
    };
    for (unsigned c = 0; c < sizeof cases / sizeof cases[0]; ++c)
        for (unsigned i = 0; i < 16; ++i)
            palette[cases[c].slot * 16 + i] = (uint16_t)(cases[c].color + i);
    for (unsigned c = 0; c < sizeof cases / sizeof cases[0]; ++c) {
        size_t offset;
        assert(mmx5_coop_duel_palette_offset(cases[c].clut, 1, sizeof palette, &offset));
        for (unsigned i = 0; i < 16; ++i)
            assert(palette[offset / 2 + i] == cases[c].color + i);
    }
}

int main(void) {
    Mmx5CoopDuelXAppearance plan, previous;
    size_t offset;
    unsigned row;
    assert(mmx5_coop_duel_x_appearance(5, &plan));
    assert(plan.record == 93 && plan.graphics_group == 0 &&
           plan.assembly_group == 0 && plan.armor_group == 5 &&
           plan.draw_roster == 4);
    assert(mmx5_coop_duel_x_appearance(19, &plan));
    assert(plan.graphics_group == 10 && plan.assembly_group == 18 &&
           plan.armor_group == 19 && plan.draw_roster == 4);
    previous = plan;
    assert(!mmx5_coop_duel_x_appearance(0, &plan));
    assert(!mmx5_coop_duel_x_appearance(18, &plan));
    assert(!mmx5_coop_duel_x_appearance(255, &plan));
    assert(!memcmp(&plan, &previous, sizeof plan));
    assert(!mmx5_coop_duel_x_appearance(5, NULL));

    native_palette_rows();
    /* These actual native CLUTs distinguish the 16-column palette layout
     * from the incorrect (clut-7800)*32 address calculation. */
    assert(mmx5_coop_duel_palette_offset(0x7800, 1, 0x4000, &offset) && offset == 0);
    assert(mmx5_coop_duel_palette_offset(0x7843, 1, 0x4000, &offset) && offset == 0x260);
    assert(mmx5_coop_duel_palette_offset(0x78C4, 4, 0x4000, &offset) && offset == 0x680);
    assert(mmx5_coop_duel_palette_offset(0x7CCC, 4, 0x4000, &offset) && offset == 0x2780);
    assert(mmx5_coop_duel_palette_offset(0xFCCC, 4, 0x4000, &offset) && offset == 0x2780);
    assert(mmx5_coop_duel_palette_offset(0x7FCF, 1, 0x4000, &offset) && offset == 0x3FE0);
    assert(mmx5_coop_duel_palette_offset(0x7CCC, 2, 0x27C0, &offset));
    offset = 0x1234;
    assert(!mmx5_coop_duel_palette_offset(0x7CCC, 2, 0x27BF, &offset));
    assert(!mmx5_coop_duel_palette_offset(0x7FCF, 2, 0x4000, &offset));
    assert(!mmx5_coop_duel_palette_offset(0x780F, 2, 0x4000, &offset));
    assert(!mmx5_coop_duel_palette_offset(0x7810, 1, 0x4000, &offset));
    assert(!mmx5_coop_duel_palette_offset(0x77C0, 1, 0x4000, &offset));
    assert(!mmx5_coop_duel_palette_offset(0x8000, 1, 0x4000, &offset));
    assert(!mmx5_coop_duel_palette_offset(0x7800, 0, 0x4000, &offset));
    assert(offset == 0x1234);
    assert(!mmx5_coop_duel_palette_offset(0x7800, 1, 0x4000, NULL));

    assert(mmx5_coop_duel_x_palette_row(0x7800, &row) && row == 0);
    assert(mmx5_coop_duel_x_palette_row(0x7843, &row) && row == 1);
    row = 99;
    assert(!mmx5_coop_duel_x_palette_row(0x7801, &row));
    assert(!mmx5_coop_duel_x_palette_row(0x7844, &row));
    assert(!mmx5_coop_duel_x_palette_row(0x7840, &row));
    assert(!mmx5_coop_duel_x_palette_row(0xF800, &row));
    assert(row == 99);
    assert(!mmx5_coop_duel_x_palette_row(0x7800, NULL));
    return 0;
}
